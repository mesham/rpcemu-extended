/*
  RPCEmu - An Acorn system emulator

  DebugCmd - expose the host-side debugger/inspector over a local socket so an
  external tool (e.g. the MCP server) can inspect and control the emulated CPU.

  Both this socket service (debugcmd_poll) and the debugger core it drives run
  on the emulator thread, so the shared state needs no locking. It is serviced
  from execrpcemu()/rpcemu_idle() while running and from MainEmuLoop() while the
  debugger has the CPU paused (so a paused CPU can still be resumed/inspected).

  Wire protocol: newline-delimited. The client sends one request line
  "<verb> [args]\n"; the server replies with exactly one JSON object line. See
  docs/debugcmd.md for the verb reference.

  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation; either version 2 of the License, or (at your option) any later
  version.
 */

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "debugcmd.h"
#include "arm.h"
#include "arm_disasm.h"
#include "cp15.h"
#include "mem.h"
#include "rpcemu.h"

#define DC_IN_BUF_SZ	512		/* one request line */
#define DC_OUT_RING_SZ	(256u * 1024u)	/* MUST be a power of two */
#define DC_RESP_SZ	(64u * 1024u)	/* max size of a single JSON response */
#define DC_MEM_MAX	4096u		/* cap bytes per mem read */
#define DC_DIS_MAX	256u		/* cap instructions per disassemble */

typedef struct {
	int	initialised;
	int	listen_fd;		/* -1 = disabled/failed */
	int	client_fd;		/* -1 = no client */
	int	is_tcp;
	char	sock_path[512];

	char	in_buf[DC_IN_BUF_SZ];
	size_t	in_len;
	int	in_overflow;

	uint8_t	out_ring[DC_OUT_RING_SZ];
	size_t	out_head;
	size_t	out_tail;
} DebugCmdState;

static DebugCmdState dc = {
	.listen_fd = -1,
	.client_fd = -1,
};

/* ---- outbound ring --------------------------------------------------- */

static size_t
dc_ring_used(void)
{
	return (dc.out_head - dc.out_tail) & (DC_OUT_RING_SZ - 1);
}

static size_t
dc_ring_free(void)
{
	return (DC_OUT_RING_SZ - 1) - dc_ring_used();
}

/* Queue a NUL-terminated response line (a '\n' is appended). Dropped whole if
   it doesn't fit, so the client never sees a truncated JSON object. */
static void
dc_send(const char *s)
{
	size_t len = strlen(s);

	if (dc.client_fd < 0) {
		return;
	}
	if (dc_ring_free() < len + 1) {
		rpclog("DebugCmd: output ring full, dropping response\n");
		return;
	}
	for (size_t i = 0; i < len; i++) {
		dc.out_ring[dc.out_head] = (uint8_t) s[i];
		dc.out_head = (dc.out_head + 1) & (DC_OUT_RING_SZ - 1);
	}
	dc.out_ring[dc.out_head] = (uint8_t) '\n';
	dc.out_head = (dc.out_head + 1) & (DC_OUT_RING_SZ - 1);
}

static void
dc_error(const char *msg)
{
	char buf[256];

	snprintf(buf, sizeof(buf), "{\"ok\":false,\"error\":\"%s\"}", msg);
	dc_send(buf);
}

/* ---- safe, side-effect-free memory access ---------------------------- */

/* Translate a virtual address to physical without leaving a data-abort event
   pending on the CPU (readmemfl/translateaddress set arm.event & 0x40 on a
   miss, which would otherwise inject a spurious abort into execution). Returns
   1 and *phys on success, 0 if unmapped. */
static int
dc_translate(uint32_t vaddr, uint32_t *phys)
{
	if (!mmu) {
		*phys = vaddr;
		return 1;
	}
	{
		uint32_t saved_event = arm.event;
		uint32_t p = translateaddress2(vaddr, 0, 0);
		int fault = (arm.event & 0x40) != 0;

		arm.event = saved_event;	/* undo any injected abort */
		if (fault) {
			return 0;
		}
		*phys = p;
		return 1;
	}
}

/* Read one byte at a virtual (default) or physical address; *mapped=0 if the
   virtual page is unmapped. Never triggers watchpoints or aborts. */
static uint8_t
dc_read8(uint32_t addr, int physical, int *mapped)
{
	uint32_t phys = addr;

	if (!physical) {
		if (!dc_translate(addr, &phys)) {
			*mapped = 0;
			return 0;
		}
	}
	*mapped = 1;
	return (uint8_t) mem_phys_read8_debug(phys);
}

static uint32_t
dc_read32(uint32_t addr, int physical, int *mapped)
{
	uint8_t b0 = dc_read8(addr, physical, mapped);
	uint8_t b1 = dc_read8(addr + 1, physical, mapped);
	uint8_t b2 = dc_read8(addr + 2, physical, mapped);
	uint8_t b3 = dc_read8(addr + 3, physical, mapped);

	return (uint32_t) b0 | ((uint32_t) b1 << 8) | ((uint32_t) b2 << 16)
	       | ((uint32_t) b3 << 24);
}

/* ---- JSON helpers ---------------------------------------------------- */

/* Append the JSON-escaped form of src to dst (bounded). */
static void
dc_json_str(char *dst, size_t dstsz, const char *src)
{
	size_t n = strlen(dst);

	for (; *src && n + 2 < dstsz; src++) {
		unsigned char c = (unsigned char) *src;

		if (c == '"' || c == '\\') {
			dst[n++] = '\\';
			dst[n++] = (char) c;
		} else if (c < 0x20) {
			n += (size_t) snprintf(dst + n, dstsz - n, "\\u%04x", c);
		} else {
			dst[n++] = (char) c;
		}
	}
	dst[n] = '\0';
}

/* ---- request handlers ------------------------------------------------ */

static void
dc_cmd_regs(char *r)
{
	size_t n;
	int i;

	n = (size_t) snprintf(r, DC_RESP_SZ,
	    "{\"ok\":true,\"paused\":%s,\"pc\":\"%08x\",\"cpsr\":\"%08x\","
	    "\"mode\":%u,\"flags\":\"%c%c%c%c\",\"regs\":[",
	    debugger_is_paused() ? "true" : "false",
	    (unsigned) PC, (unsigned) arm.reg[cpsr], (unsigned) arm.mode,
	    (arm.reg[cpsr] & NFLAG) ? 'N' : '-',
	    (arm.reg[cpsr] & ZFLAG) ? 'Z' : '-',
	    (arm.reg[cpsr] & CFLAG) ? 'C' : '-',
	    (arm.reg[cpsr] & VFLAG) ? 'V' : '-');
	for (i = 0; i < 16; i++) {
		n += (size_t) snprintf(r + n, DC_RESP_SZ - n, "%s\"%08x\"",
		    i ? "," : "", (unsigned) arm.reg[i]);
	}
	snprintf(r + n, DC_RESP_SZ - n, "]}");
}

static void
dc_cmd_status(char *r)
{
	DebuggerStatus st;
	size_t n;
	uint32_t i;

	debugger_get_status(&st);
	n = (size_t) snprintf(r, DC_RESP_SZ,
	    "{\"ok\":true,\"paused\":%s,\"pause_requested\":%s,\"reason\":%u,"
	    "\"halt_pc\":\"%08x\",\"halt_opcode\":\"%08x\",\"last_pc\":\"%08x\","
	    "\"hit_address\":\"%08x\",\"hit_value\":\"%08x\",\"hit_size\":%u,"
	    "\"hit_is_write\":%u,\"step_active\":%u,\"trace_pending\":%u,"
	    "\"breakpoints\":[",
	    st.paused ? "true" : "false", st.pause_requested ? "true" : "false",
	    (unsigned) st.reason, (unsigned) st.halt_pc, (unsigned) st.halt_opcode,
	    (unsigned) st.last_pc, (unsigned) st.hit_address, (unsigned) st.hit_value,
	    (unsigned) st.hit_size, (unsigned) st.hit_is_write,
	    (unsigned) st.step_active, (unsigned) debugger_trace_pending());
	for (i = 0; i < st.breakpoint_count; i++) {
		n += (size_t) snprintf(r + n, DC_RESP_SZ - n, "%s\"%08x\"",
		    i ? "," : "", (unsigned) st.breakpoints[i]);
	}
	n += (size_t) snprintf(r + n, DC_RESP_SZ - n, "],\"watchpoints\":[");
	for (i = 0; i < st.watchpoint_count; i++) {
		n += (size_t) snprintf(r + n, DC_RESP_SZ - n,
		    "%s{\"address\":\"%08x\",\"size\":%u,\"on_read\":%u,"
		    "\"on_write\":%u,\"log_only\":%u}",
		    i ? "," : "", (unsigned) st.watchpoints[i].address,
		    (unsigned) st.watchpoints[i].size, st.watchpoints[i].on_read,
		    st.watchpoints[i].on_write, st.watchpoints[i].log_only);
	}
	snprintf(r + n, DC_RESP_SZ - n, "]}");
}

static void
dc_cmd_mem(char *r, char *args)
{
	char *a1 = strtok(args, " \t");
	char *a2 = strtok(NULL, " \t");
	char *a3 = strtok(NULL, " \t");
	uint32_t addr, len, i;
	int physical;
	size_t n;

	if (!a1 || !a2) {
		dc_error("usage: mem <hexaddr> <len> [phys]");
		return;
	}
	addr = (uint32_t) strtoul(a1, NULL, 16);
	len = (uint32_t) strtoul(a2, NULL, 0);
	physical = (a3 && (a3[0] == 'p' || a3[0] == 'P'));
	if (len > DC_MEM_MAX) {
		len = DC_MEM_MAX;
	}
	n = (size_t) snprintf(r, DC_RESP_SZ,
	    "{\"ok\":true,\"addr\":\"%08x\",\"physical\":%s,\"len\":%u,\"data\":\"",
	    (unsigned) addr, physical ? "true" : "false", (unsigned) len);
	for (i = 0; i < len && n + 3 < DC_RESP_SZ; i++) {
		int mapped;
		uint8_t b = dc_read8(addr + i, physical, &mapped);

		n += (size_t) snprintf(r + n, DC_RESP_SZ - n, "%02x", b);
	}
	snprintf(r + n, DC_RESP_SZ - n, "\"}");
}

static void
dc_cmd_dis(char *r, char *args)
{
	char *a1 = strtok(args, " \t");
	char *a2 = strtok(NULL, " \t");
	char *a3 = strtok(NULL, " \t");
	uint32_t addr, count, i;
	int physical;
	size_t n;

	if (!a1) {
		dc_error("usage: dis <hexaddr> [count] [phys]");
		return;
	}
	addr = (uint32_t) strtoul(a1, NULL, 16);
	count = a2 ? (uint32_t) strtoul(a2, NULL, 0) : 16;
	physical = (a3 && (a3[0] == 'p' || a3[0] == 'P'));
	if (count == 0 || count > DC_DIS_MAX) {
		count = (count == 0) ? 1 : DC_DIS_MAX;
	}
	n = (size_t) snprintf(r, DC_RESP_SZ, "{\"ok\":true,\"lines\":[");
	for (i = 0; i < count; i++) {
		uint32_t a = addr + i * 4;
		int mapped;
		uint32_t opcode = dc_read32(a, physical, &mapped);
		char dis[128];
		char line[192];

		if (!mapped) {
			snprintf(line, sizeof(line), "%08x: <unmapped>", (unsigned) a);
		} else {
			arm_disasm(opcode, a, dis, sizeof(dis));
			snprintf(line, sizeof(line), "%08x: %08x  %s",
			    (unsigned) a, (unsigned) opcode, dis);
		}
		n += (size_t) snprintf(r + n, DC_RESP_SZ - n, "%s\"", i ? "," : "");
		dc_json_str(r, DC_RESP_SZ, line);
		n = strlen(r);
		n += (size_t) snprintf(r + n, DC_RESP_SZ - n, "\"");
	}
	snprintf(r + n, DC_RESP_SZ - n, "]}");
}

static void
dc_cmd_bp(char *r, char *args)
{
	char *sub = strtok(args, " \t");
	char *a1 = strtok(NULL, " \t");
	uint32_t addr;

	if (!sub) {
		dc_error("usage: bp add|del|clear [hexaddr]");
		return;
	}
	if (strcmp(sub, "clear") == 0) {
		debugger_clear_breakpoints();
		snprintf(r, DC_RESP_SZ, "{\"ok\":true}");
		return;
	}
	if (!a1) {
		dc_error("usage: bp add|del <hexaddr>");
		return;
	}
	addr = (uint32_t) strtoul(a1, NULL, 16);
	if (strcmp(sub, "add") == 0) {
		int ok = debugger_add_breakpoint(addr);

		snprintf(r, DC_RESP_SZ, "{\"ok\":%s,\"address\":\"%08x\"%s}",
		    ok ? "true" : "false", (unsigned) addr,
		    ok ? "" : ",\"error\":\"breakpoint table full\"");
	} else if (strcmp(sub, "del") == 0) {
		debugger_remove_breakpoint(addr);
		snprintf(r, DC_RESP_SZ, "{\"ok\":true,\"address\":\"%08x\"}", (unsigned) addr);
	} else {
		dc_error("usage: bp add|del|clear [hexaddr]");
	}
}

static void
dc_cmd_wp(char *r, char *args)
{
	char *sub = strtok(args, " \t");
	char *a1 = strtok(NULL, " \t");
	char *a2 = strtok(NULL, " \t");
	char *a3 = strtok(NULL, " \t");
	char *a4 = strtok(NULL, " \t");
	uint32_t addr, size;
	int on_read, on_write, log_only;

	if (!sub) {
		dc_error("usage: wp add|del|clear [hexaddr size r|w|rw [log]]");
		return;
	}
	if (strcmp(sub, "clear") == 0) {
		debugger_clear_watchpoints();
		snprintf(r, DC_RESP_SZ, "{\"ok\":true}");
		return;
	}
	if (!a1 || !a2 || !a3) {
		dc_error("usage: wp add|del <hexaddr> <size> <r|w|rw> [log]");
		return;
	}
	addr = (uint32_t) strtoul(a1, NULL, 16);
	size = (uint32_t) strtoul(a2, NULL, 0);
	on_read = (strchr(a3, 'r') != NULL || strchr(a3, 'R') != NULL);
	on_write = (strchr(a3, 'w') != NULL || strchr(a3, 'W') != NULL);
	log_only = (a4 && a4[0] == 'l');
	if (strcmp(sub, "add") == 0) {
		int ok = debugger_add_watchpoint(addr, size, on_read, on_write, log_only);

		snprintf(r, DC_RESP_SZ, "{\"ok\":%s%s}", ok ? "true" : "false",
		    ok ? "" : ",\"error\":\"watchpoint table full or invalid\"");
	} else if (strcmp(sub, "del") == 0) {
		debugger_remove_watchpoint(addr, size, on_read, on_write);
		snprintf(r, DC_RESP_SZ, "{\"ok\":true}");
	} else {
		dc_error("usage: wp add|del|clear ...");
	}
}

static void
dc_cmd_trace(char *r, char *args)
{
	char *a1 = strtok(args, " \t");
	uint32_t max = a1 ? (uint32_t) strtoul(a1, NULL, 0) : 128;
	DebugTraceEvent ev[128];
	uint32_t dropped = 0, got, i;
	size_t n;

	if (max == 0 || max > 128) {
		max = 128;
	}
	got = debugger_drain_trace_events(ev, max, &dropped);
	n = (size_t) snprintf(r, DC_RESP_SZ,
	    "{\"ok\":true,\"dropped\":%u,\"events\":[", (unsigned) dropped);
	for (i = 0; i < got; i++) {
		n += (size_t) snprintf(r + n, DC_RESP_SZ - n,
		    "%s{\"seq\":%u,\"type\":%u,\"pc\":\"%08x\",\"opcode\":\"%08x\","
		    "\"arg0\":\"%08x\",\"arg1\":\"%08x\",\"arg2\":\"%08x\"}",
		    i ? "," : "", (unsigned) ev[i].seq, (unsigned) ev[i].type,
		    (unsigned) ev[i].pc, (unsigned) ev[i].opcode, (unsigned) ev[i].arg0,
		    (unsigned) ev[i].arg1, (unsigned) ev[i].arg2);
	}
	snprintf(r + n, DC_RESP_SZ - n, "]}");
}

static void
dc_dispatch(char *line)
{
	static char resp[DC_RESP_SZ];
	char *verb;
	char *args;

	/* trim trailing CR/whitespace */
	{
		size_t l = strlen(line);
		while (l > 0 && (line[l - 1] == '\r' || line[l - 1] == ' '
		    || line[l - 1] == '\t')) {
			line[--l] = '\0';
		}
	}
	verb = strtok(line, " \t");
	args = strtok(NULL, "");	/* rest of line */
	if (!verb) {
		return;			/* blank line: ignore */
	}
	resp[0] = '\0';

	if (strcmp(verb, "ping") == 0) {
		snprintf(resp, DC_RESP_SZ,
		    "{\"ok\":true,\"paused\":%s,\"model\":\"%s\",\"dynarec\":%s}",
		    debugger_is_paused() ? "true" : "false",
		    models[machine.model].name_config, arm_is_dynarec() ? "true" : "false");
	} else if (strcmp(verb, "regs") == 0) {
		dc_cmd_regs(resp);
	} else if (strcmp(verb, "status") == 0) {
		dc_cmd_status(resp);
	} else if (strcmp(verb, "mem") == 0) {
		dc_cmd_mem(resp, args ? args : (char *) "");
	} else if (strcmp(verb, "dis") == 0) {
		dc_cmd_dis(resp, args ? args : (char *) "");
	} else if (strcmp(verb, "bp") == 0) {
		dc_cmd_bp(resp, args ? args : (char *) "");
	} else if (strcmp(verb, "wp") == 0) {
		dc_cmd_wp(resp, args ? args : (char *) "");
	} else if (strcmp(verb, "trace") == 0) {
		dc_cmd_trace(resp, args ? args : (char *) "");
	} else if (strcmp(verb, "pause") == 0) {
		debugger_request_pause(DebugPauseReason_User);
		snprintf(resp, DC_RESP_SZ, "{\"ok\":true,\"paused\":%s}",
		    debugger_is_paused() ? "true" : "false");
	} else if (strcmp(verb, "resume") == 0 || strcmp(verb, "continue") == 0) {
		debugger_resume();
		snprintf(resp, DC_RESP_SZ, "{\"ok\":true,\"paused\":false}");
	} else if (strcmp(verb, "step") == 0) {
		char *a1 = args ? strtok(args, " \t") : NULL;
		uint32_t nsteps = a1 ? (uint32_t) strtoul(a1, NULL, 0) : 1;

		if (nsteps == 0) {
			nsteps = 1;
		}
		debugger_single_step(nsteps);
		snprintf(resp, DC_RESP_SZ, "{\"ok\":true,\"stepped\":%u}", (unsigned) nsteps);
	} else {
		dc_error("unknown verb");
	}

	if (resp[0] != '\0') {
		dc_send(resp);
	}
}

/* ---- socket lifecycle (mirrors hostcmd.c) ---------------------------- */

static void
dc_set_nonblock(int fd)
{
	int fl = fcntl(fd, F_GETFL, 0);

	if (fl >= 0) {
		fcntl(fd, F_SETFL, fl | O_NONBLOCK);
	}
}

static int
dc_listen_unix(const char *path)
{
	struct sockaddr_un addr;
	int fd;

	if (strlen(path) >= sizeof(addr.sun_path)) {
		rpclog("DebugCmd: socket path too long: %s\n", path);
		return -1;
	}
	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		rpclog("DebugCmd: socket() failed: %s\n", strerror(errno));
		return -1;
	}
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
	unlink(path);
	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		rpclog("DebugCmd: bind(%s) failed: %s\n", path, strerror(errno));
		close(fd);
		return -1;
	}
	if (listen(fd, 1) < 0) {
		rpclog("DebugCmd: listen() failed: %s\n", strerror(errno));
		close(fd);
		unlink(path);
		return -1;
	}
	dc_set_nonblock(fd);
	strncpy(dc.sock_path, path, sizeof(dc.sock_path) - 1);
	dc.sock_path[sizeof(dc.sock_path) - 1] = '\0';
	dc.is_tcp = 0;
	rpclog("DebugCmd: listening on AF_UNIX %s\n", path);
	return fd;
}

static int
dc_listen_tcp(int port)
{
	struct sockaddr_in addr;
	int fd;
	int on = 1;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		rpclog("DebugCmd: socket() failed: %s\n", strerror(errno));
		return -1;
	}
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons((uint16_t) port);
	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		rpclog("DebugCmd: bind(127.0.0.1:%d) failed: %s\n", port, strerror(errno));
		close(fd);
		return -1;
	}
	if (listen(fd, 1) < 0) {
		rpclog("DebugCmd: listen() failed: %s\n", strerror(errno));
		close(fd);
		return -1;
	}
	dc_set_nonblock(fd);
	dc.is_tcp = 1;
	dc.sock_path[0] = '\0';
	rpclog("DebugCmd: listening on TCP 127.0.0.1:%d\n", port);
	return fd;
}

void
debugcmd_init(void)
{
	if (dc.initialised) {
		debugcmd_close();
	}
	memset(&dc, 0, sizeof(dc));
	dc.listen_fd = -1;
	dc.client_fd = -1;
	dc.initialised = 1;

	if (!config.debug_enabled) {
		return;
	}
	if (config.debug_socket[0] == '\0' || config.debug_socket[0] == '/') {
		char path[512];

		if (config.debug_socket[0] == '/') {
			strncpy(path, config.debug_socket, sizeof(path) - 1);
			path[sizeof(path) - 1] = '\0';
		} else {
			snprintf(path, sizeof(path), "%srpcemu-debug.sock",
			    rpcemu_get_datadir());
		}
		dc.listen_fd = dc_listen_unix(path);
	} else {
		int port = atoi(config.debug_socket);

		if (port > 0 && port < 65536) {
			dc.listen_fd = dc_listen_tcp(port);
		} else {
			rpclog("DebugCmd: invalid socket spec '%s', disabling\n",
			    config.debug_socket);
		}
	}
}

static void
dc_drop_client(void)
{
	if (dc.client_fd >= 0) {
		close(dc.client_fd);
		dc.client_fd = -1;
	}
	dc.in_len = 0;
	dc.in_overflow = 0;
	dc.out_head = dc.out_tail = 0;
}

void
debugcmd_reset(void)
{
	if (!dc.initialised) {
		return;
	}
	dc.in_len = 0;
	dc.in_overflow = 0;
}

void
debugcmd_close(void)
{
	if (dc.client_fd >= 0) {
		close(dc.client_fd);
		dc.client_fd = -1;
	}
	if (dc.listen_fd >= 0) {
		close(dc.listen_fd);
		dc.listen_fd = -1;
	}
	if (!dc.is_tcp && dc.sock_path[0] != '\0') {
		unlink(dc.sock_path);
		dc.sock_path[0] = '\0';
	}
	dc.initialised = 0;
}

/* ---- per-tick service ------------------------------------------------ */

static void
dc_read_client(void)
{
	uint8_t tmp[1024];
	ssize_t nread;
	ssize_t i;

	nread = recv(dc.client_fd, tmp, sizeof(tmp), 0);
	if (nread == 0) {
		dc_drop_client();
		return;
	}
	if (nread < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return;
		}
		dc_drop_client();
		return;
	}
	for (i = 0; i < nread; i++) {
		uint8_t b = tmp[i];

		if (b == '\n') {
			if (!dc.in_overflow) {
				dc.in_buf[dc.in_len] = '\0';
				dc_dispatch(dc.in_buf);
			} else {
				dc_error("request line too long");
			}
			dc.in_len = 0;
			dc.in_overflow = 0;
			continue;
		}
		if (dc.in_len < DC_IN_BUF_SZ - 1) {
			dc.in_buf[dc.in_len++] = (char) b;
		} else {
			dc.in_overflow = 1;
			dc.in_len = 0;
		}
	}
}

static void
dc_write_client(void)
{
	while (dc_ring_used() > 0) {
		size_t used = dc_ring_used();
		size_t to_end = DC_OUT_RING_SZ - dc.out_tail;
		size_t contig = (used < to_end) ? used : to_end;
		ssize_t nwr = send(dc.client_fd, &dc.out_ring[dc.out_tail], contig,
		    MSG_NOSIGNAL);

		if (nwr < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return;
			}
			dc_drop_client();
			return;
		}
		if (nwr == 0) {
			return;
		}
		dc.out_tail = (dc.out_tail + (size_t) nwr) & (DC_OUT_RING_SZ - 1);
	}
}

void
debugcmd_poll(void)
{
	struct pollfd pfd;

	if (dc.listen_fd < 0) {
		return;
	}
	if (dc.client_fd < 0) {
		pfd.fd = dc.listen_fd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		if (poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN)) {
			int c = accept(dc.listen_fd, NULL, NULL);

			if (c >= 0) {
				dc_set_nonblock(c);
				dc.client_fd = c;
				dc.in_len = 0;
				dc.in_overflow = 0;
				dc.out_head = dc.out_tail = 0;
			}
		}
		return;
	}
	pfd.fd = dc.client_fd;
	pfd.events = POLLIN;
	if (dc_ring_used() > 0) {
		pfd.events |= POLLOUT;
	}
	pfd.revents = 0;
	if (poll(&pfd, 1, 0) <= 0) {
		return;
	}
	if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
		dc_drop_client();
		return;
	}
	if (pfd.revents & POLLIN) {
		dc_read_client();
		if (dc.client_fd < 0) {
			return;
		}
	}
	if (pfd.revents & POLLOUT) {
		dc_write_client();
	}
}
