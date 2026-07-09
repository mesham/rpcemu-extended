/*
  RPCEmu - An Acorn system emulator

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
