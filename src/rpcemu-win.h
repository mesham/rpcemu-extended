/*
  RPCEmu - An Acorn system emulator

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
