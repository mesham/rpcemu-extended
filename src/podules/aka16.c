/*
  RPCEmu - An Acorn system emulator

  Copyright (C) Sarah Walker

  Part of the podule subsystem, derived from Arculator 2.2 by Sarah Walker
  (https://b-em.bbcmicro.com/arculator/), and distributed under the GNU GPL v2.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*Acorn AKA16 MIDI Podule

  IOC address map :
  0000-1fff - ROM
  2000-2fff - SCC2691 UART
  3000-3fff - ROM banking
*/
//#define DEBUG_LOG

#ifdef WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "scc2691.h"
#include "midi.h"
#include "podule_api.h"

#ifdef WIN32
extern __declspec(dllexport) const podule_header_t *podule_probe(const podule_callbacks_t *callbacks, char *path);
#else
#define BOOL int
#define APIENTRY
#endif

static const podule_callbacks_t *podule_callbacks;
static char podule_path[512];

#define MIDI_UART_CLOCK 2000000 //(31250Hz * 4 * 16)

#ifdef DEBUG_LOG
static FILE *aka16_logf;
#endif

void aka16_log(const char *format, ...)
{
#ifdef DEBUG_LOG
   char buf[1024];
//return;
   if (!aka16_logf) aka16_logf=fopen("aka16_log.txt","wt");
   va_list ap;
   va_start(ap, format);
   vsprintf(buf, format, ap);
   va_end(ap);
   fputs(buf,aka16_logf);
   fflush(aka16_logf);
#endif
}

typedef struct aka16_t
{
	uint8_t rom[0x4000];
	int rom_page;

	scc2691_t scc2691;

	podule_t *podule;
	void *midi;
} aka16_t;

static uint8_t aka16_read_b(struct podule_t *podule, podule_io_type type, uint32_t addr)
{
	aka16_t *aka16 = podule->p;
	uint8_t temp = 0xff;

	if (type != PODULE_IO_TYPE_IOC)
		return 0xff;

	//aka16_log("aka16_read_b: addr=%04x\n", addr);
	switch (addr&0x3000)
	{
		case 0x0000: case 0x1000:
		//aka16_log("  rom_page=%i rom_addr=%04x\n", aka16->rom_page, ((aka16->rom_page * 2048) + ((addr & 0x1fff) >> 2)) & 0x3fff);
		return aka16->rom[((aka16->rom_page * 2048) + ((addr & 0x1fff) >> 2)) & 0x3fff];

		case 0x2000:
		return scc2691_read(&aka16->scc2691, (addr >> 2) & 7);
	}
	return 0xFF;
}

static void aka16_write_b(struct podule_t *podule, podule_io_type type, uint32_t addr, uint8_t val)
{
	aka16_t *aka16 = podule->p;

	if (type != PODULE_IO_TYPE_IOC)
		return;

	aka16_log("aka16_write_b: addr=%04x val=%02x\n", addr, val);
	switch (addr & 0x3000)
	{
		case 0x2000: /*SCC2691*/
		scc2691_write(&aka16->scc2691, (addr >> 2) & 7, val);
		break;
		case 0x3000:
		aka16->rom_page = val;
		break;
	}
}

static int aka16_run(struct podule_t *podule, int timeslice_us)
{
	aka16_t *aka16 = podule->p;

	scc2691_run(&aka16->scc2691, timeslice_us);

	return 256; /*256us = 1 byte at 31250 baud*/
}

static void aka16_uart_irq(void *p, int state)
{
	aka16_t *aka16 = p;
	podule_t *podule = aka16->podule;

	podule_callbacks->set_irq(podule, state);
}

static void aka16_uart_send(void *p, uint8_t val)
{
	struct aka16_t *aka16 = p;

	midi_write(aka16->midi, val);
}

static void aka16_midi_receive(void *p, uint8_t val)
{
	struct aka16_t *aka16 = p;

	scc2691_receive(&aka16->scc2691, val);
}

static int aka16_init(struct podule_t *podule)
{
	FILE *f;
	char rom_fn[512];
	aka16_t *aka16 = malloc(sizeof(aka16_t));
	memset(aka16, 0, sizeof(aka16_t));

	sprintf(rom_fn, "%spodules/aka16/aka16.rom", podule_path);
	aka16_log("aka16 ROM %s\n", rom_fn);
	f = fopen(rom_fn, "rb");
	if (!f)
	{
		aka16_log("Failed to open aka16.ROM!\n");
		return -1;
	}
	fread(aka16->rom, 0x4000, 1, f);
	fclose(f);

	scc2691_init(&aka16->scc2691, MIDI_UART_CLOCK, aka16_uart_irq, aka16_uart_send, aka16, aka16_log);

	aka16->midi = midi_init(aka16, aka16_midi_receive, aka16_log, podule_callbacks, podule);

	aka16->podule = podule;
	podule->p = aka16;

	return 0;
}

static void aka16_close(struct podule_t *podule)
{
	aka16_t *aka16 = podule->p;

	midi_close(aka16->midi);

	free(aka16);
}

static podule_config_t aka16_config =
{
	.items =
	{
		{
			.name = "midi_out_device",
			.description = "MIDI output device",
			.type = CONFIG_SELECTION,
			.selection = NULL,
			.default_int = -1
		},
		{
			.name = "midi_in_device",
			.description = "MIDI input device",
			.type = CONFIG_SELECTION,
			.selection = NULL,
			.default_int = -1
		},
		{
			.type = -1
		}
	}
};

static const podule_header_t aka16_podule_header =
{
	.version = PODULE_API_VERSION,
	.flags = PODULE_FLAGS_UNIQUE,
	.short_name = "aka16",
	.name = "Acorn AKA16 MIDI Podule",
	.functions =
	{
		.init = aka16_init,
		.close = aka16_close,
		.read_b = aka16_read_b,
		.write_b = aka16_write_b,
		.run = aka16_run
	},
	.config = &aka16_config
};

const podule_header_t *aka16_probe(const podule_callbacks_t *callbacks, char *path)
{
	podule_callbacks = callbacks;
	strcpy(podule_path, path);

	aka16_config.items[0].selection = midi_out_devices_config();
	aka16_config.items[1].selection = midi_in_devices_config();

	return &aka16_podule_header;
}

/* Only when this podule is built as a standalone loadable DLL; in the emulator
   these podules are compiled into the core, where a per-file DllMain collides. */
#if defined(WIN32) && defined(RPCEMU_PODULE_STANDALONE_DLL)
BOOL APIENTRY DllMain (HINSTANCE hInst     /* Library instance handle. */ ,
		       DWORD reason        /* Reason this function is being called. */ ,
		       LPVOID reserved     /* Not used. */ )
{
    switch (reason)
    {
      case DLL_PROCESS_ATTACH:
	break;

      case DLL_PROCESS_DETACH:
	break;

      case DLL_THREAD_ATTACH:
	break;

      case DLL_THREAD_DETACH:
	break;
    }

    /* Returns TRUE on success, FALSE on failure */
    return TRUE;
}
#endif
