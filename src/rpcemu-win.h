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

/*
  Windows (MinGW-w64) compatibility shims for the POSIX-oriented core.

  Include this AFTER the standard system headers (in particular <sys/stat.h>)
  in translation units that use the POSIX filesystem calls the core relies on.
  On non-Windows targets it expands to nothing.
*/
#ifndef RPCEMU_WIN_H
#define RPCEMU_WIN_H

#ifdef _WIN32

#include <direct.h>
#include <io.h>

/* POSIX mkdir() takes a permission mode; the Windows CRT's _mkdir() does not.
   The core only ever passes 0777, which is meaningless on Windows anyway. */
#ifdef mkdir
#undef mkdir
#endif
#define mkdir(path, mode) _mkdir(path)

#endif /* _WIN32 */

#endif /* RPCEMU_WIN_H */
