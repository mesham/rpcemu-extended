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

#ifndef GUI_PREFERENCES_H
#define GUI_PREFERENCES_H

#include <string>
#include <vector>

static const int MaxRecentMachines = 5;
static const int MaxRecentFloppies = 10;
static const int MaxRecentCDROMs = 10;

std::vector<std::string> GetRecentMachines();
void AddRecentMachine(const std::string &machine_name);
void ClearRecentMachines();

std::vector<std::string> GetRecentFloppies();
void AddRecentFloppy(const std::string &path);
void ClearRecentFloppies();

std::vector<std::string> GetRecentCDROMs();
void AddRecentCDROM(const std::string &path);
void ClearRecentCDROMs();

#endif
