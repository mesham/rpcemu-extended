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

typedef struct am7202a_t
{
	int rp, wp;
	uint16_t data[1024];

	void *p;

	void (*set_ef)(int state, void *p);
	void (*set_ff)(int state, void *p);
	void (*set_hf)(int state, void *p);
} am7202a_t;

void am7202a_init(am7202a_t *fifo, void (*set_ef)(int state, void *p), void (*set_ff)(int state, void *p), void (*set_hf)(int state, void *p), void *p);
void am7202a_reset(am7202a_t *fifo);
void am7202a_write(am7202a_t *fifo, uint16_t val);
uint16_t am7202a_read(am7202a_t *fifo);
