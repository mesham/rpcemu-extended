/*
  RPCEmu - An Acorn system emulator

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 */

#ifndef EDID_H
#define EDID_H

#include <stdint.h>

#define EDID_BLOCK_SIZE 128

/**
 * Build a 128-byte EDID block advertising a preferred (native) mode plus a
 * ladder of common lower modes, starting from an existing known-good block.
 *
 * The header, feature and DPMS bytes are taken from @base unchanged (RISC OS's
 * VIDC20 driver depends on some of them); only the timing sections and the
 * checksum are rewritten. The block is left so the driver's own sync-bit fixup
 * (which adjusts byte 0x14 and the checksum at runtime) keeps the checksum
 * valid.
 *
 * @param out    Destination 128-byte block
 * @param base   Source 128-byte block to inherit non-timing fields from
 * @param x      Preferred mode width in pixels
 * @param y      Preferred mode height in pixels
 * @param hz     Preferred mode vertical refresh in Hz
 */
void edid_build_from_base(uint8_t out[EDID_BLOCK_SIZE],
                          const uint8_t base[EDID_BLOCK_SIZE],
                          unsigned x, unsigned y, unsigned hz);

/**
 * Validate that a 128-byte buffer is a structurally sound EDID 1.x block
 * (correct 8-byte header, version 1, byte-sum a multiple of 256).
 *
 * @param block 128-byte candidate
 * @return non-zero if the block is a valid EDID
 */
int edid_block_is_valid(const uint8_t block[EDID_BLOCK_SIZE]);

#endif
