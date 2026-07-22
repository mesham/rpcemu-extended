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

/*Acorn AKA05 ROM Podule

  Ported to RPCEmu from Arculator 2.2 (Sarah Walker). This device depends only
  on the podule plugin ABI (podule_api.h), demonstrating that Arculator-style
  podules can be hosted by RPCEmu unchanged in structure.

  IOC address map :
    0000-1fff - ROM
    2000-2fff - latch
      bits 0-5 : ROM A11-A16
      bits 6-7 : ROM select 0-1
    3000-3fff - PAL
      A11 : ROM select 2
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include "podule_api.h"

/*#define DEBUG_LOG*/

static const podule_callbacks_t *podule_callbacks;
static char podule_path[512];

static void
aka05_log(const char *format, ...)
{
#ifdef DEBUG_LOG
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
#else
	(void)format;
#endif
}

typedef struct aka05_t
{
	uint8_t *roms[8];
	uint32_t rom_mask[8];
	int rom_writable[8];

	int rom_page;
	int rom_select;

	podule_t *podule;
} aka05_t;

static uint8_t
aka05_read_b(struct podule_t *podule, podule_io_type type, uint32_t addr)
{
	aka05_t *aka05 = podule->p;

	if (type != PODULE_IO_TYPE_IOC) {
		return 0xff;
	}

	switch (addr & 0x3000) {
	case 0x0000: case 0x1000:
		if (aka05->roms[aka05->rom_select]) {
			uint32_t rom_addr = ((aka05->rom_page * 2048) + ((addr & 0x1fff) >> 2));

			return aka05->roms[aka05->rom_select][rom_addr & aka05->rom_mask[aka05->rom_select]];
		}
		return 0xff; /*No ROM present in this slot*/
	}
	return 0xff;
}

static void
aka05_write_b(struct podule_t *podule, podule_io_type type, uint32_t addr, uint8_t val)
{
	aka05_t *aka05 = podule->p;

	if (type != PODULE_IO_TYPE_IOC) {
		return;
	}

	switch (addr & 0x3000) {
	case 0x0000: case 0x1000:
		if (aka05->roms[aka05->rom_select] && aka05->rom_writable[aka05->rom_select]) {
			uint32_t rom_addr = ((aka05->rom_page * 2048) + ((addr & 0x1fff) >> 2));

			aka05->roms[aka05->rom_select][rom_addr & aka05->rom_mask[aka05->rom_select]] = val;
		}
		break;

	case 0x2000:
		aka05->rom_page = val & 0x3f;
		aka05->rom_select = (aka05->rom_select & 4) | ((val >> 6) & 3);
		break;

	case 0x3000:
		if (addr & 0x800) {
			aka05->rom_select |= 4;
		} else {
			aka05->rom_select &= ~4;
		}
		break;
	}
}

static int
aka05_init(struct podule_t *podule)
{
	FILE *f;
	char rom_fn[512];
	aka05_t *aka05 = malloc(sizeof(aka05_t));

	if (aka05 == NULL) {
		return -1;
	}
	memset(aka05, 0, sizeof(aka05_t));

	/*Manager ROM is fixed - and required*/
	snprintf(rom_fn, sizeof(rom_fn), "%spodules/aka05/rom_podule_0.07.bin", podule_path);
	aka05_log("aka05 ROM %s\n", rom_fn);
	f = fopen(rom_fn, "rb");
	if (!f) {
		aka05_log("Failed to open %s\n", rom_fn);
		free(aka05);
		return -1;
	}
	aka05->roms[0] = malloc(0x4000);
	if (aka05->roms[0] == NULL || fread(aka05->roms[0], 0x4000, 1, f) != 1) {
		free(aka05->roms[0]);
		free(aka05);
		fclose(f);
		return -1;
	}
	aka05->rom_mask[0] = 0x3fff;
	fclose(f);

	/*Remaining ROMs are loaded from config (no-op until config store exists)*/
	for (int i = 1; i < 6; i++) {
		char config_name[16];

		snprintf(config_name, sizeof(config_name), "rom_fn%i", i + 1);

		const char *fn = podule_callbacks->config_get_string(podule, config_name, NULL);

		if (fn) {
			f = fopen(fn, "rb");
			if (f) {
				uint32_t size, mask;

				fseek(f, -1, SEEK_END);
				size = ftell(f) + 1;
				fseek(f, 0, SEEK_SET);

				if (size <= 0x2000)
					mask = 0x1fff;
				else if (size <= 0x4000)
					mask = 0x3fff;
				else if (size <= 0x8000)
					mask = 0x7fff;
				else if (size <= 0x10000)
					mask = 0xffff;
				else
					mask = 0x1ffff; /*Maximum size = 128k*/

				aka05->roms[i] = malloc(mask + 1);
				aka05->rom_mask[i] = mask;

				if (aka05->roms[i] == NULL || fread(aka05->roms[i], mask + 1, 1, f) != 1) {
					free(aka05->roms[i]);
					aka05->roms[i] = NULL;
				}
				fclose(f);
			}
		}
	}

	if (podule_callbacks->config_get_int(podule, "ram_7", 0)) {
		aka05->roms[6] = malloc(0x8000);
		aka05->rom_mask[6] = 0x7fff;
		aka05->rom_writable[6] = 1;
	}
	if (podule_callbacks->config_get_int(podule, "ram_8", 0)) {
		aka05->roms[7] = malloc(0x8000);
		aka05->rom_mask[7] = 0x7fff;
		aka05->rom_writable[7] = 1;
	}

	aka05->rom_select = 0;
	aka05->podule = podule;
	podule->p = aka05;

	return 0;
}

static void
aka05_close(struct podule_t *podule)
{
	aka05_t *aka05 = podule->p;

	if (aka05 == NULL) {
		return;
	}
	for (int i = 0; i < 8; i++) {
		free(aka05->roms[i]);
	}
	free(aka05);
	podule->p = NULL;
}

static const podule_header_t aka05_podule_header =
{
	.version = PODULE_API_VERSION,
	.flags = 0,
	.short_name = "aka05",
	.name = "Acorn AKA05 ROM Podule",
	.functions =
	{
		.init = aka05_init,
		.close = aka05_close,
		.read_b = aka05_read_b,
		.write_b = aka05_write_b
	},
	.config = NULL
};

const podule_header_t *
aka05_probe(const podule_callbacks_t *callbacks, char *path)
{
	podule_callbacks = callbacks;
	snprintf(podule_path, sizeof(podule_path), "%s", path);

	return &aka05_podule_header;
}
