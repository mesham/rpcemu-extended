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

struct lark_t;

#define AD1848_NCoef 91

typedef struct ad1848_t
{
	uint8_t config;

	int index;
	uint8_t regs[16];
	uint8_t status;

	int trd;
	int mce;

	int count;

	double timing_error_us;
	double sample_time_us;

	int sample_freq;

	int first_poll;
	int16_t sound_in_buffer[256*1024*2];
	int16_t sound_out_buffer[4800*2];
	uint32_t rp, fp, wp;
	uint32_t samp_inc;
	int rate_divider;

	double output_iir_coef_a[3];
	double output_iir_coef_b[3];


	int16_t in_buffer_raw[4800*2];
	int in_buffer_pending_samples;

	int16_t in_buffer[8192*2*2];
	int16_t filter_buffer[8192*16*2];
	uint32_t in_rp, in_wp;
	uint32_t in_ip, in_i_inc;
	uint32_t in_f_wp;

	int sound_time;

	int playback_enable;
	int capture_enable;

	float low_fir_coef[AD1848_NCoef];

	struct lark_t *lark;

	uint8_t (*dma_read)(struct lark_t *lark);
	void (*dma_write)(struct lark_t *lark, uint8_t val);
} ad1848_t;

uint8_t ad1848_read(ad1848_t *ad1848, uint16_t addr);
void ad1848_write(ad1848_t *ad1848, uint16_t addr, uint8_t val);
void ad1848_init(ad1848_t *ad1848, struct lark_t *lark, uint8_t (*dma_read)(struct lark_t *lark), void (*dma_write)(struct lark_t *lark, uint8_t val));
void ad1848_run(ad1848_t *ad1848, int timeslice_us);
void ad1848_in_buffer(ad1848_t *ad1848, int16_t *buffer, int samples);
