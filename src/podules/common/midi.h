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

#include "podule_api.h"

void *midi_init(void *p, void (*receive)(void *p, uint8_t val), void (*log)(const char *format, ...), const podule_callbacks_t *podule_callbacks, podule_t *podule);
void midi_close(void *p);
void midi_write(void *p, uint8_t val);

podule_config_selection_t *midi_out_devices_config(void);
podule_config_selection_t *midi_in_devices_config(void);
