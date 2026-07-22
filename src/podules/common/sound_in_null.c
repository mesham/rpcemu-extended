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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "podule_api.h"
#include "sound_in.h"

typedef struct wave_in_t
{
	void *p;
} wave_in_t;

void *sound_in_init(void *p, void (*sound_in_buffer)(void *p, void *buffer, int samples), void (*log)(const char *format, ...), const podule_callbacks_t *podule_callbacks, podule_t *podule)
{
	wave_in_t *wave_in = malloc(sizeof(wave_in_t));
	memset(wave_in, 0, sizeof(wave_in_t));

	wave_in->p = p;

	return wave_in;
}

void sound_in_close(void *p)
{
	wave_in_t *wave_in = p;

	free(wave_in);
}

void sound_in_start(void *p)
{
}
void sound_in_stop(void *p)
{
}

podule_config_selection_t *sound_in_devices_config(void)
{
	int nr_devs;
	podule_config_selection_t *sel;
	podule_config_selection_t *sel_p;
	char *in_dev_text = malloc(65536);
	int c;

	nr_devs = 0;
	sel = malloc(sizeof(podule_config_selection_t) * (nr_devs+2));
	sel_p = sel;

	strcpy(in_dev_text, "None");
	sel_p->description = in_dev_text;
	sel_p->value = -1;
	sel_p++;
	in_dev_text += strlen(in_dev_text)+1;

	strcpy(in_dev_text, "");
	sel_p->description = in_dev_text;

	return sel;
}
