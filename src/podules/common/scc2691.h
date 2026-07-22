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

typedef struct scc2691_t
{
	uint8_t isr, imr;

	int tx_irq_pending;

	uint8_t rx_queue[256];
	int rx_rp, rx_wp;
	int rx_pending;
	uint8_t rx_data;

	int input_clock;
	int baud_rate;

	void (*set_irq)(void *p, int state);
	void (*tx_data)(void *p, uint8_t val);
	void (*log)(const char *format, ...);
	void *p;
} scc2691_t;

void scc2691_init(scc2691_t *scc2691, int input_clock, void (*set_irq)(void *p, int state), void (*tx_data)(void *p, uint8_t val), void *p, void (*log)(const char *format, ...));
uint8_t scc2691_read(scc2691_t *scc2691, uint32_t addr);
void scc2691_write(scc2691_t *scc2691, uint32_t addr, uint8_t val);
void scc2691_run(scc2691_t *scc2691, int timeslice_us);

void scc2691_receive(scc2691_t *scc2691, uint8_t val);
