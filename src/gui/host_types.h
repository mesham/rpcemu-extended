/*
  RPCEmu - An Acorn system emulator

  Copyright (C) 2025-2026 Andy Timmins

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

#ifndef HOST_TYPES_H
#define HOST_TYPES_H

#include <cstdint>

struct VideoUpdate {
	const uint32_t *buffer = nullptr;
	int xsize = 0;
	int ysize = 0;
	int yl = 0;
	int yh = 0;
	int double_size = 0;
	int host_xsize = 0;
	int host_ysize = 0;
};

struct MouseMoveUpdate {
	int16_t x = 0;
	int16_t y = 0;
};

#endif
