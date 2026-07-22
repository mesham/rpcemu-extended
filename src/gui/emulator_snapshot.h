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

#ifndef EMULATOR_SNAPSHOT_H
#define EMULATOR_SNAPSHOT_H

#include <cstdint>
#include <string>
#include <vector>

#include "machine_snapshot.h"

MachineSnapshot emulator_take_snapshot();
std::vector<uint8_t> emulator_read_memory(uint32_t address, uint32_t length);
std::string emulator_disassemble_at(uint32_t address, int count);

#endif
