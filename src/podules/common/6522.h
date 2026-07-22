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
typedef struct via6522_t
{
	uint8_t  ora,   orb,   ira,   irb;
	uint8_t  ddra,  ddrb;
	uint8_t  sr;
	uint32_t t1l,   t2l;
	int      t1c,   t2c;
	uint8_t  acr,   pcr,   ifr,   ier;
	int      t1hit, t2hit;
	int      ca1,   ca2,   cb1,   cb2;
	int      intnum;

	uint8_t  (*read_portA)(void *p);
	uint8_t  (*read_portB)(void *p);
	void     (*write_portA)(void *p, uint8_t val);
	void     (*write_portB)(void *p, uint8_t val);

	void     (*set_ca1)(void *p, int level);
	void     (*set_ca2)(void *p, int level);
	void     (*set_cb1)(void *p, int level);
	void     (*set_cb2)(void *p, int level);

	void (*set_irq)(void *p, int state);
	void *p;
} via6522_t;

void    via6522_init(via6522_t *v, void (*set_irq)(void *p, int state), void *p);
uint8_t via6522_read(via6522_t *v, uint16_t addr);
void    via6522_write(via6522_t *v, uint16_t addr, uint8_t val);
void    via6522_updatetimers(via6522_t *v, int time);

void via6522_set_ca1(via6522_t *v, int level);
void via6522_set_ca2(via6522_t *v, int level);
void via6522_set_cb1(via6522_t *v, int level);
void via6522_set_cb2(via6522_t *v, int level);
