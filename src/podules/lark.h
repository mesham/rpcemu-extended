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

#ifndef _DLL_H_
#define _DLL_H_

#if BUILDING_DLL
# define DLLIMPORT __declspec (dllexport)
#else /* Not BUILDING_DLL */
# define DLLIMPORT __declspec (dllimport)
#endif /* Not BUILDING_DLL */

void lark_log(const char *format, ...);

#define IRQ_MASTER (1 << 0)
#define IRQ_AD1848 (1 << 1)
#define IRQ_16550  (1 << 2)
#define IRQ_OUT_FIFO (1 << 6)
#define IRQ_IN_FIFO  (1 << 7)

struct lark_t;
void lark_set_irq(struct lark_t *lark, uint8_t irq);
void lark_clear_irq(struct lark_t *lark, uint8_t irq);

void lark_midi_send(struct lark_t *lark, uint8_t val);

void lark_sound_in_start(struct lark_t *lark);
void lark_sound_in_stop(struct lark_t *lark);

void lark_sound_out_buffer(struct lark_t *lark, void *buffer, int samples);

int wss_irq();

#endif /* _DLL_H_ */
