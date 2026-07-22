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
  DebugCmd - expose the host-side debugger/inspector over a local socket.

  Emulator (host) half of the debugger control channel. The socket service runs
  on the emulator thread (like HostCmd), so it can call the debugger_* / memory
  / disassembler functions directly with no locking. See docs/debugcmd.md.
 */

#ifndef RPCEMU_DEBUGCMD_H
#define RPCEMU_DEBUGCMD_H

#ifdef __cplusplus
extern "C" {
#endif

extern void debugcmd_init(void);
extern void debugcmd_poll(void);
extern void debugcmd_reset(void);
extern void debugcmd_close(void);

#ifdef __cplusplus
}
#endif

#endif /* RPCEMU_DEBUGCMD_H */
