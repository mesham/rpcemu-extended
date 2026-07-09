# DebugCmd — control the emulated CPU from the host

DebugCmd exposes RPCEmu's host-side debugger/inspector over a local socket, so
an external tool (the [MCP server](../tools/mcp/README.md), an IDE, a script)
can inspect and control the **emulated ARM CPU** — read registers and memory,
disassemble, set breakpoints and watchpoints, and single-step. It is the
programmatic counterpart to the GUI Machine Inspector.

```
host tool (rpcemu MCP server / your script)
        │   local socket (AF_UNIX or TCP 127.0.0.1)
        ▼
   emulator (debugcmd.c)  ──▶  debugger core + arm register file + memory + disassembler
```

Both the socket service and the debugger core run on the **emulator thread**, so
DebugCmd calls the `debugger_*` API, `mem_phys_read8_debug()`, `translateaddress2()`
and `arm_disasm()` directly with no locking. The socket is serviced while the CPU
is running *and* while it is paused, so a paused CPU can always be resumed.

## Configuration

Per-machine `.cfg` keys (under `[General]`):

| Key | Default | Meaning |
| --- | --- | --- |
| `debug_enabled` | `1` | Enable the DebugCmd socket. |
| `debug_socket` | *(empty)* | Empty ⇒ `<data-dir>/rpcemu-debug.sock` (AF_UNIX). A path ⇒ that AF_UNIX path. A bare port number ⇒ TCP on `127.0.0.1:<port>`. |

> **Security.** Whoever can open this socket can halt the emulated CPU and read
> its memory. The default transport is an AF_UNIX socket under the machine's data
> directory (filesystem-permission limited, never on the network); the optional
> TCP mode binds `127.0.0.1` only. A paused CPU freezes the whole machine until
> resumed. Set `debug_enabled=0` to disable.

## Wire protocol

Newline-delimited, request/response. The client sends one request line
`<verb> [args]\n`; the server replies with exactly one JSON object line. All
addresses are hex; numeric args accept decimal or `0x`-prefixed hex. Every
response has an `"ok"` boolean; failures carry `"error"`.

| Request | Response (JSON) |
| --- | --- |
| `ping` | `{ok, paused, model, dynarec}` |
| `regs` | `{ok, paused, pc, cpsr, mode, flags, regs:[16 hex]}` |
| `status` | `{ok, paused, pause_requested, reason, halt_pc, halt_opcode, last_pc, hit_address, hit_value, hit_size, hit_is_write, step_active, trace_pending, breakpoints:[hex], watchpoints:[{address,size,on_read,on_write,log_only}]}` |
| `mem <addr> <len> [phys]` | `{ok, addr, physical, len, data:"<hex>"}` — `len` capped 4096; virtual unless `phys` |
| `dis <addr> [count] [phys]` | `{ok, lines:["<addr>: <opcode>  <mnemonic>"]}` — `count` capped 256 |
| `bp add\|del <addr>` / `bp clear` | `{ok, address}` / `{ok}` |
| `wp add\|del <addr> <size> <r\|w\|rw> [log]` / `wp clear` | `{ok}` |
| `pause` | `{ok, paused}` — pause is **deferred** (takes effect at the next instruction); poll `status` |
| `resume` (alias `continue`) | `{ok, paused:false}` |
| `step [n]` | `{ok, stepped}` — steps `n` instructions then re-pauses |
| `trace [max]` | `{ok, dropped, events:[{seq,type,pc,opcode,arg0,arg1,arg2}]}` — `max` capped 128 |

`status.reason` / pause reasons: `0`=none `1`=user `2`=breakpoint `3`=watchpoint
`4`=step `5`=exception `6`=SWI. Trace `type`: `0`=exception `1`=SWI `2`=watchpoint.

## Safety notes

- **Memory reads and disassembly are side-effect-free.** Virtual addresses are
  translated with the CPU's data-abort event saved/restored (so a read of an
  unmapped page can't inject a spurious abort into execution), then read via the
  no-side-effect `mem_phys_read8_debug()` — they never fire watchpoints.
- **Breakpoints/watchpoints and pause/step change execution.** A breakpoint or
  non-log watchpoint hit pauses the CPU (freezing the machine) until `resume`.
- Limits: 64 breakpoints, 32 watchpoints (mirrors the GUI inspector).
