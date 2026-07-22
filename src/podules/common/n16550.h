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

typedef struct n16550_t
{
	uint8_t iir, ier;
	uint8_t int_status;
	uint8_t mctrl;
	uint8_t dlab1, dlab2;
	uint8_t fcr, lcr;
	uint8_t lsr;
	uint8_t msr;
	uint8_t scratch;
	uint8_t thr;

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
} n16550_t;

void n16550_init(n16550_t *n16550, int input_clock, void (*set_irq)(void *p, int state), void (*tx_data)(void *p, uint8_t val), void *p, void (*log)(const char *format, ...));
uint8_t n16550_read(n16550_t *n16550, uint32_t addr);
void n16550_write(n16550_t *n16550, uint32_t addr, uint8_t val);
void n16550_run(n16550_t *n16550, int timeslice_us);

void n16550_receive(n16550_t *n16550, uint8_t val);
