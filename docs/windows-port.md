# Windows port plan

Bringing RPCEmu Spork Edition back to Windows. The repo went Linux-only in commit
`6828f03` ("Modernise to Linux/wxWidgets/CMake"), which **replaced the Qt5 GUI with
wxWidgets** and, in the same stroke, deleted the old Windows platform layer
(`src/win/rpc-win.c`, `network-win.c`, `tap-win32.c`, `hostfs-win.c`,
`src/qt5/keyboard_win.c`, the WiX installer). Those files are recoverable from git
history as *reference* but are Qt5-era and not drop-in.

The key consequence: **wxWidgets is cross-platform, so the GUI does not need
rewriting.** The remaining work is conventional POSIX→Win32 shimming in the C core
plus build-system plumbing. There is no architectural blocker.

## Progress

**The entire C core (M1 core + M2 dynarec + M3 networking + §6 podules/CD-ROM)
is ported and verified.** Every core translation unit (72 of them) cross-compiles
clean for Windows with MinGW-w64 (`x86_64-w64-mingw32-gcc`), and the native Linux
build is unchanged (no regressions). What remains for a bootable Windows build is
the GUI layer (§5) and packaging/CI (§7), which need a real MSYS2 environment
(wxWidgets-msw + SDL2) to compile and run — neither is available on the Linux dev
box, so they are the next milestone rather than done work.

**Verification method.** A MinGW-w64 cross-compiler on the Linux dev box compiles
each core `.c` to a Windows object file with the same flags as the CMake
`rpcemu_core` target. This empirically proves the POSIX→Win32 shimming compiles;
it does *not* exercise the GUI, the final link (needs wxWidgets/SDL2/libvncserver
for the cross target), or runtime (no wine). Those require the M-later MSYS2 build.

**Done + cross-compile-verified:**
- **M2 dynarec** — `set_memory_executable()` now has a `_WIN32` branch using
  `VirtualProtect`/`GetSystemInfo`; the `#error "requires Linux"` is gone.
- **§3 filesystem** — `<dirent.h>` *is* present in MinGW-w64, so the
  `opendir`/`readdir` sites (hostfs/romload/podulerom) needed **no change** and a
  separate `hostfs-win.c` was **not** required. Only `mkdir(path,0777)` needed a
  shim (`_mkdir`, via new `src/rpcemu-win.h`), plus `statvfs`→`GetDiskFreeSpaceEx`
  and `uname`→static string in `rpc.c`, and `clock_gettime`→`GetTickCount64` in
  `peripheral_config.c`. The `rpcemu.c` idle path now keys off `_WIN32` (→`Sleep`).
- **§4 networking** — new `src/socket-compat.h` gives both platforms the right
  socket headers, `closesocket`, a `socket_set_nonblocking()` helper, `poll`→
  `WSAPoll`, `MSG_NOSIGNAL`, and a `sock_errno()`/`SOCK_E*` layer (Winsock reports
  via `WSAGetLastError()`, not `errno` — the non-blocking send/recv loops depended
  on this). hostcmd/debugcmd skip AF_UNIX on Windows and default to a TCP loopback
  port (15590/15591). broadcast_relay's `getifaddrs` has a `GetAdaptersInfo`
  (iphlpapi) branch. `WSAStartup`/`WSACleanup` bracket `rpcemu_start`/`endrpcemu`.
  `RPCEMU_NETWORKING` now covers `_WIN32` (it gates NAT init + the SWI dispatch).
  Bridged/tunnelled networking is stubbed out via new `src/network-null.c`.
- **§6 podules** — `dlopen`/`dlsym`/`dlclose`→`LoadLibrary`/`GetProcAddress`/
  `FreeLibrary` shim in `podules.c`; loadable-podule extension `.so`→`.dll`.
  `cdrom-ioctl.c` (Linux ioctl backend, unreferenced) is excluded on Windows; the
  portable `cdrom-iso.c` is the default.
- **§1 build system** — the Linux `FATAL_ERROR` gate now permits `WIN32` (and
  rejects MSVC); GCC-only flags guarded behind `NOT MSVC`; core sources are
  platform-split (Windows drops `network-tun.c`/`cdrom-ioctl.c`, adds
  `network-null.c`); `ws2_32`+`iphlpapi` linked on Windows;
  `__USE_MINGW_ANSI_STDIO=1` set so `%zu`/`%lld` work in logging (with a matching
  `RPCEMU_FORMAT_PRINTF` archetype macro in `rpcemu.h`). New cross toolchain file
  `cmake/mingw-w64-x86_64.cmake`.

**GUI + CI — the full GUI now cross-compiles from Linux and links to
`rpcemu-recompiler.exe` (PE32+), verified on the dev box:**
- **§5 GUI** — `gui/CMakeLists.txt` requires GTK3 on non-Windows only (it is used
  solely by the `__WXGTK__` mouse path) and guards the `.desktop` install;
  `data_paths.cpp` uses `wxGetHomeDir()` (→`%USERPROFILE%\RPCEmu`) instead of
  `getenv("HOME")`. Issues the cross-build surfaced and fixed: `headless_main.cpp`
  `ExeDir()` used `readlink("/proc/self/exe")` → now `GetModuleFileNameA` on
  Windows; `emulator_host.cpp` called `ioctl_init()` (Linux CD-ROM backend)
  unguarded → now `__linux__`-only; the built-in podules aka12/aka16/lark each
  defined a `DllMain` under `#ifdef WIN32` that collided when compiled into the
  static core → now gated behind `RPCEMU_PODULE_STANDALONE_DLL` (never set in the
  core build). `keyboard_x.c` is a pure data table (no X11); GDK is
  `__WXGTK__`-guarded; wx is linked statically.
- **cross-compiling from Linux** — `setup-cross-build-env.sh` builds the mingw
  dependencies (wxWidgets 3.2.6 wxMSW static, SDL2, libvncserver + zlib/png/jpeg)
  into `/usr/x86_64-w64-mingw32`; `cmake/FindWxWidgets.cmake` uses the cross
  `wx-config` directly when `CMAKE_CROSSCOMPILING` (CMake's win32-style
  FindwxWidgets can't see a `--host` wx install); `cmake/mingw-w64-x86_64.cmake`
  pins `PKG_CONFIG_LIBDIR` to the sysroot. To reproduce:
  `bash setup-cross-build-env.sh` then
  `cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake -DRPCEMU_BUILD_TESTS=OFF -DRPCEMU_ENABLE_GHOSTPDL=OFF && cmake --build build-win -j`.
  The `.exe` links wx statically but needs the MinGW/SDL2/libvncserver runtime
  DLLs bundled to *run* (packaging, §7). Runtime on Windows is still unverified
  (no wine); the CI job builds natively on MSYS2 as the second check.
- **tools** — `rpcemu_run.c` (`rpcemu-run`/`rpcemu-shell`) fully ported and
  cross-compile-verified (winsock, TCP-only default `127.0.0.1:15590`,
  `read`/`write`→`recv`/`send`); `rpcemu-shell` ships as a `.exe` copy on Windows
  (no symlink).
- **§7 packaging** — `build-windows.sh` stages a runnable release into
  `releases/windows/amd64/` (parity with `build.sh`'s `releases/linux/<arch>/`):
  same resource dirs (configs/poduleroms/netroms/resources/roms/podules/default/
  machines) + tools/mcp + docs, the three `.exe`s, and the **transitive runtime
  DLL closure** bundled beside them (SDL2, libvncserver, zlib1, libjpeg-62,
  libgcc_s_seh-1, libstdc++-6, libwinpthread-1 — libpng is static in
  libvncserver). `--zip` writes `releases/windows/rpcemu_<ver>_windows_amd64.zip`.
  The script is dual-mode: Linux cross (default) or native MSYS2 (`$MSYSTEM`),
  so the CI reuses it.
- **§7 CI** — the `windows-amd64` MSYS2 (MINGW64) job runs
  `./build-windows.sh --zip`, PE-smoke-tests the exe, and uploads
  `releases/windows/` — parity with the Linux jobs' tarball upload. It is **not
  yet wired into the `release` job's `needs`/`files`** — do that (add
  `windows-amd64` to `needs` and `upload/**/*.zip` to `files`) once the CI build
  is proven green, so it can publish Windows zips alongside the Linux .deb/.tar.gz
  without an unproven build blocking Linux releases.

**First run on real Windows (2026-07-11) — runtime issues found & partly fixed:**
- **wxMSW `wx.rc` was not compiled in** → startup asserts "Loading a cursor
  defined by wxWidgets failed ... include wx/msw/wx.rc", no app manifest (so no
  comctl32 v6 theming). FIXED: `FindWxWidgets.cmake` now compiles
  `#include "wx/msw/wx.rc"` (via windres, with the wx include dirs) into the exe
  on Windows — `.rsrc` + manifest now embedded. Applies to both cross and native.
- **No `wxInitAllImageHandlers()`** → PNG bitmaps don't load on wxMSW. FIXED in
  `main.cpp` OnInit.
- **Toolbar icons still partly broken (OPEN):** `toolbar_icons.cpp` falls back to
  `wxArtProvider::GetBitmap(wxART_FLOPPY/CDROM/REFRESH/STOP/GO_FORWARD/…, wxART_TOOLBAR)`
  on non-GTK, but wxMSW's art provider has no bitmap for several of those IDs →
  blank/broken. Needs embedded XPM/PNG fallbacks for all toolbar icons (the
  GTK-theme path is Linux-only). Polish.
- **Black emulator display (OPEN, functional blocker):** CPU runs (MIPS counter
  live) but the panel stays black. Paint path is portable wx
  (`wxBufferedPaintDC`+`StretchBlit`), so suspect ROM/boot (resource-dir
  resolution) or the cross-thread VIDC→GUI video posting under winpthreads.
  Needs `rpclog.txt` from a Windows run to diagnose.

**Black display DIAGNOSED (2026-07-11): the dynarec is System-V-ABI-only.**
`codegen_amd64.c` emits JIT→C helper calls with SysV arg registers (`MOV
$opcode,%edi` = arg1 in RDI; documented `%edi/%esi/%edx` = args 1/2/3). The
Windows x64 ABI passes args in RCX/RDX/R8/R9 (+32B shadow space) and makes
RSI/RDI callee-saved. So every JIT helper call (`readmemfl/fb`, `writememfl/fb`,
`arm_load/store_multiple`) is mis-ABI'd on Windows. Symptom: runs at ~60 MIPS
(hot paths are inlined in the JIT) but RISC OS stalls the instant it needs a
helper — e.g. handling the first (harmless) early-boot data abort. Log proof:
Linux continues past the abort to `HostFS: Registration request accepted`;
Windows logs nothing after the abort. The fc008b40/fault_addr=0x8000 data abort
itself is the known-harmless ROM app-space probe.

**Resolution (interim): Windows ships the INTERPRETER.** `arm.c`+`codegen_null.c`
are pure C, ABI-agnostic, cross-compile clean, and boot RISC OS correctly (same
as the arm64 Linux release). `build-windows.sh` builds the interpreter by default
(`--dynarec` opt-in for later); binary is `rpcemu-interpreter.exe`.

**Dynarec on Windows (remaining full-speed parity work):** the JIT calls only ~6
helpers, so the tractable fix is `__attribute__((sysv_abi))` on those helpers +
the JIT block entry (so the whole JIT world stays SysV), gated on Windows — NOT a
full codegen rewrite. Needs on-Windows (or wine) testing. Verify the ~6 helpers
aren't also called from MS-ABI C code in a dynarec build before annotating.

**Also fixed this session:** wxMSW `wx.rc` (cursors/manifest) now compiled in;
`wxInitAllImageHandlers()` added. Keyboard works (X11 raw-path gated to wxGTK; K/L
keycode table bug fixed). Mouse works in "mouse follows" mode. **Toolbar icons:**
every button now has an embedded 24×24 XPM fallback (`toolbar_icons.cpp`), so
wxMSW no longer depends on `wxArtProvider` IDs it lacks; wxGTK still prefers the
native theme, so Linux is unchanged.

**Icons:** an SVG toolbar pack (`src/gui/icons/*.svg`) is embedded into the binary
(CMake `file(READ HEX)` → generated header) and rendered via
`wxBitmapBundle::FromSVG` on both platforms. App icon (`resources/rpcemu.png`)
drives the window/taskbar icon (both), the Windows `.exe` icon (compiled-in
`resources/rpcemu.ico`), and the Linux `.desktop`/hicolor icon.

**CI:** the `windows-amd64` MSYS2 job builds + PE-smoke-tests + uploads the zip,
and is wired into the `release` job (`needs` + `upload/**/*.zip`), so a `vX.Y.Z`
tag publishes the Windows zip alongside the Linux `.deb`/`.tar.gz`.

**Still to do:** dynarec Windows-ABI port (above, for full-speed parity); §5 M4
parity (full Windows scancode→PS/2 table + relative-mouse capture); optionally
harden `PostVideoUpdate` to be fully async (a snapshot-during-reset could still
deadlock the sync video path).

New/changed files: `src/socket-compat.h`, `src/rpcemu-win.h`, `src/network-null.c`,
`cmake/mingw-w64-x86_64.cmake` (new); `CMakeLists.txt`, `src/CMakeLists.txt`,
`src/rpcemu.h`, `arm_dynarec.c`, `rpc.c`, `rpc-machdep.c`, `printer.c`, `hostfs.c`,
`peripheral_config.c`, `hostcmd.c`, `debugcmd.c`, `broadcast_relay.c`, `podules.c`,
`rpcemu.c` (changed).

## Decisions (locked)

- **Toolchain: MinGW-w64 via MSYS2.** Not MSVC. MinGW keeps the codebase's GCC
  assumptions working unchanged — pthreads, `__attribute__((aligned))`, the
  `__amd64__`/`__i386__` codegen macros, `-std=gnu11`, `-fno-common`,
  `-Werror=switch`. Under MSVC every one of those is a separate rewrite; under
  MinGW they build as-is.
- **Target: full parity.** Dynarec (JIT) enabled, full networking, full keyboard
  scancode fidelity — not a cut-down interpreter release. The milestones below are
  an *engineering build order*, not shipping intermediate reduced builds.
- **Networking config: NAT/SLiRP + TCP-loopback control sockets.** No kernel
  driver, no admin rights. Bridged TUN/TAP is explicitly out of scope (see
  §"Out of scope").

## Toolchain setup (MSYS2 / MinGW-w64)

Build from the **MINGW64** shell. Packages (names are `mingw-w64-x86_64-*`):

- `toolchain` (gcc, binutils, make), `cmake`, `pkgconf`
- `wxwidgets3.2-msw` (or current wx 3.2 package)
- `SDL2` (host audio — already used via `plt_sound.cpp`)
- `libvncserver` (optional, for the VNC server behind `RPCEMU_VNC`)
- SLiRP is **bundled** (`src/slirp/`), no package needed

The C runtime linkage that matters: link `ws2_32` (Winsock) and `iphlpapi`
(adapter enumeration) — see networking section.

## Build order (milestones toward the full-parity release)

1. **M1 — build unblocks + interpreter boots.** Remove the Linux gate, guard
   GCC-only flags, wire deps, port the core filesystem/loader shims. Prove the
   full wxWidgets GUI + core stack boots RISC OS on Windows with
   `RPCEMU_DYNAREC=OFF`. This validates everything *except* the JIT.
2. **M2 — dynarec enabled.** Port `set_memory_executable()` to `VirtualProtect`
   and drop the `#error`. Full-speed emulation. (Under MinGW the codegen macros
   and aligned array already work; this milestone is essentially one function.)
3. **M3 — networking.** WSAStartup bootstrap, NAT/SLiRP up, hostcmd/debugcmd on
   TCP loopback, `getifaddrs`→`GetAdaptersAddresses` for the broadcast relay.
4. **M4 — parity polish.** Windows scancode→PS/2 table (full keyboard fidelity),
   wxMSW relative-mouse capture, loadable podules via `LoadLibrary`.
5. **M5 — packaging + CI.** NSIS/ZIP CPack generator, `windows-latest` GitHub
   Actions job, PE smoke test.

---

## Work breakdown by layer

Checkboxes are the actual tasks. File:line references are from the codebase audit
(2026-07-10) — verify before editing, line numbers drift.

### 1. Build system

- [ ] Remove/relax the hard Linux gate: `CMakeLists.txt:18-20`
  (`if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux") FATAL_ERROR`).
- [ ] Guard GCC-only compile flags behind `if(NOT MSVC)` (harmless under MinGW,
  but keep clean): `-Wall -Wextra -Werror=switch -fno-common`
  (`CMakeLists.txt:22-24`), `-std=gnu11` + `_DEFAULT_SOURCE`
  (`src/CMakeLists.txt:104-105`). MinGW accepts these, so this is low-risk.
- [ ] Dependency discovery: SDL2 / libvncserver / GTK3 are found via
  `pkg_check_modules` (`src/gui/CMakeLists.txt:69-79`, `src/CMakeLists.txt:118-125`).
  MSYS2 ships `pkgconf`, so these mostly work — but **make GTK3 conditional on
  `__WXGTK__`** (see GUI §5; wxMSW must not require GTK3). wxWidgets already uses
  `find_package` (`cmake/FindWxWidgets.cmake`) — portable as-is.
- [ ] Ghostscript/GhostPDL discovery assumes `.so`/`.a` + `/usr/include` layout
  (`src/CMakeLists.txt:153-176`). Already behind `RPCEMU_ENABLE_GHOSTPDL` — keep
  it OFF on Windows initially.
- [ ] `rpcemu-shell` is installed as a **symlink** (`src/tools/CMakeLists.txt:11-16`,
  both POST_BUILD and `install(CODE create_symlink)`). Windows has no reliable
  symlinks — replace with a copy or a `.bat` wrapper.
- [ ] `build.sh` and `setup-build-env.sh` are bash + `apt` + `.deb`
  (`setup-build-env.sh:33` hard-requires apt) — **do not extend these**; add a
  separate Windows build path (documented CMake invocation, later a `.ps1`).

### 2. Dynarec / JIT executable memory  (M2)

- [ ] `set_memory_executable()` — `arm_dynarec.c:562-578` is wrapped
  `#if defined(__linux__) ... #else #error "RPCEmu dynarec requires Linux"`.
  Add a `defined(_WIN32)` branch: `VirtualProtect(ptr, len, PAGE_EXECUTE_READWRITE,
  &old)` (page-align via `GetSystemInfo().dwPageSize` instead of
  `sysconf(_SC_PAGESIZE)`).
- [ ] Include guard `arm_dynarec.c:31-34` (`__linux__ || __MACH__` → `<sys/mman.h>`):
  add a Windows branch including `<windows.h>`.
- Notes that make this small:
  - The JIT buffer is a **static BSS array** (`codegen_amd64.c:39`
    `rcodeblock[BLOCKS][1792] __attribute__((aligned(4096)))`), not `mmap`'d.
    `VirtualProtect` flips a static array's protection exactly like `mprotect`.
    The `aligned` attribute compiles unchanged under MinGW.
  - It's **W+X** (whole region RWX, no per-write re-protect toggling) and there's
    **no SIGSEGV/fault-based paging** — so no SEH handler is needed. Pure primitive
    swap.
  - Backend selection macros `arm_dynarec.c:39-43` use `__amd64__`/`__i386__` —
    **MinGW-GCC defines these**, so no change needed (MSVC would need `_M_X64`).
  - CMake arch match already accepts `AMD64` (`src/CMakeLists.txt:73-84`), the value
    Windows reports.

### 3. Core filesystem & loader shims  (M1 — the largest chunk)

The machine-dependent layer is split across `rpc-machdep.c` (paths/dirs),
`rpc.c` (OS queries), and `hostfs-unix.c` (HostFS stat/time backend). A Windows
build needs a `hostfs-win.c` twin plus scattered POSIX→Win32 swaps.

- [ ] **New `src/hostfs-win.c`** — twin of `hostfs-unix.c`. Port the stat/time
  translation: `utimensat`/`utime` (`hostfs-unix.c:116,209`) → `SetFileTime`/
  `_utime`; `stat` (`:230`) → `_stat`/`GetFileAttributesEx`; RISC OS ↔ Windows
  file-time conversion (FILETIME epoch differs).
- [ ] `hostfs.c` — POSIX dir/file ops throughout: `opendir`/`readdir`/`closedir`
  (`:617-679`, `:1841-1898`), `unlink` (`:1460`), `rmdir` (`:1468`),
  `mkdir(...,0777)` (`:1567`), `rename` (`:1274,1378,1773`). Replace the dir
  scanning with `FindFirstFile`/`FindNextFile` (or a small `dirent` compat shim —
  MSYS2/MinGW may provide `<dirent.h>`, verify). `PATH_MAX` (~30 buffers) →
  `MAX_PATH`. All `fopen`s already use binary mode ("rb"/"wb"/"rb+") so **no CRLF
  surprises**.
- [ ] `rpc-machdep.c` — `mkdir(path, 0777)` (`:27`) → `_mkdir(path)`; hardcoded `/`
  separators (`:44,74,79-84`) — accept `\` or normalize.
- [ ] `romload.c` (`:274-326,461-500`) and `podulerom.c` (`:160-187`) — same
  `opendir`/`readdir`/`stat`/`S_ISDIR` pattern; share the dir-scan shim.
- [ ] `rpc.c` — `statvfs` (`:50`) → `GetDiskFreeSpaceEx`; `uname` (`:76`) →
  `GetVersionEx` or a static string. Remove dead POSIX includes (`:23-30`).
- [ ] `printer.c` — `gettimeofday` (`:89`) → `GetSystemTimeAsFileTime`/
  `QueryPerformanceCounter`; `mkdir(...,0777)` (`:114`) → `_mkdir`.
- [ ] `peripheral_config.c:253-260` — `clock_gettime(CLOCK_MONOTONIC)` in
  `modem_now_ms()` → `QueryPerformanceCounter` (or `GetTickCount64`).
- [ ] `rpcemu.c:1005-1015` — there's already a stubbed `#ifdef RPCEMU_WIN` +
  `Sleep(1)` idle path, but `RPCEMU_WIN` is never defined and `windows.h` never
  included. Define the macro for Windows builds and include the header; the active
  Linux path uses `nanosleep`.

### 4. Networking  (M3)

Recommended config: **NAT/SLiRP for the guest + TCP-loopback for the control
sockets.** No kernel driver.

- [ ] **WSAStartup bootstrap** — nothing calls it today (Linux never needed it).
  Add `WSAStartup()`/`WSACleanup()` at process init and link `ws2_32`. Must run
  before SLiRP init and before any socket in hostcmd/debugcmd.
- [ ] **SLiRP / NAT** — the bundled `src/slirp/` is **already `_WIN32`-ready**
  (QEMU-derived; Winsock guards throughout, e.g. `slirp/slirp.h:9-28`,
  `slirp_config.h`, `socket.c:465`). NAT mode (`network-nat.c`) needs only build
  wiring + WSAStartup. Verify the `select` glue in `network_nat_poll`
  (`network-nat.c:256-284`) — fds must be SOCKETs; SLiRP handles its own internally.
- [ ] **hostcmd / debugcmd on TCP loopback** — both already have a TCP-127.0.0.1
  fallback (`hostcmd.c:315-333` / `debugcmd.c:554-570`) selected when the config
  value is a bare port number (`hostcmd.c:359-383`). On Windows, **skip the AF_UNIX
  branch** (`hostcmd.c:273-304`, `debugcmd.c:512-543`) — AF_UNIX on Win10+ lacks fs
  permission semantics and the `unlink()`-on-teardown is meaningless — and default
  the config to a port. Zero new socket code to get running.
- [ ] **Winsock shims** across `hostcmd.c`, `debugcmd.c`, `broadcast_relay.c`:
  - `close()`→`closesocket()` on socket fds — ~15 sites (hostcmd
    `:291,296,327,332,390,424,428`; debugcmd `:530,535,566,571,622,644,648`;
    tun helper `network-tun.c:197`). `broadcast_relay.c` already macro-abstracts
    this (`relay_closesocket`, `:55`) — just redefine for Winsock.
  - `fcntl(O_NONBLOCK)`→`ioctlsocket(FIONBIO)` (hostcmd `:263-266`,
    debugcmd `:502-505`, relay `:273-274`).
  - `poll()`→`WSAPoll()` (hostcmd `:528-568`, debugcmd `:729-757`) — same
    `pollfd`/`POLLIN` shape; wrap or fall back to `select` to dodge WSAPoll's
    historical POLLOUT quirk.
  - `MSG_NOSIGNAL` (hostcmd `:509`, debugcmd `:710`) → define as `0` (Windows never
    raises SIGPIPE). Drop `MSG_DONTWAIT` (relay `:633`; socket is already
    non-blocking).
  - `setsockopt` optval → cast to `char*` (hostcmd `:320`, debugcmd `:559`).
  - Socket-path error logging: `strerror(errno)` → `WSAGetLastError()`.
- [ ] **broadcast_relay interface enumeration** — `getifaddrs()`/`struct ifaddrs`
  (`broadcast_relay.c:157-206`, `<ifaddrs.h>` `:49`) has no Windows equivalent →
  `GetAdaptersAddresses` (iphlpapi). NAT calls `broadcast_relay_poll()`
  (`network-nat.c:283`), so this is needed for full NAT function — or stub the
  relay initially (NAT works without host broadcast forwarding).
- [ ] **VNC server** (`src/gui/vnc_server.cpp`, behind `RPCEMU_VNC`) — no raw
  sockets of its own; built on libvncserver + `std::thread`, both cross-platform.
  Windows work is "get libvncserver to link," no source changes. It sets
  `serverFormat.bigEndian = FALSE` (`:43`) — fine.
- Byte order: all fields use `htonl`/`htons`/manual big-endian assembly — **already
  portable**, `winsock2.h` provides the byte-swap functions.

### 5. GUI glue  (mostly M1, parity items M4)

wxWidgets carries the GUI. Three real Windows items, plus paths.

- [ ] **Threading** — `emulator_host.cpp` drives the sound + VIDC worker threads
  with **raw pthreads** (`:13,55-214,520`, incl. `pthread_setname_np`). Under
  **MinGW-w64 pthreads exist**, so this **compiles unchanged for M1** — no rewrite
  needed. (Optional later hardening: convert to `std::thread`/`std::mutex`/
  `std::condition_variable`, already partly included at `:11,41-47`, to shed the
  pthread dependency. Not required for MinGW parity.)
- [ ] **Relative-mouse capture** — `emulator_panel.cpp:49-52` calls raw GDK
  (`gtk_widget_add_events(... GDK_POINTER_MOTION_MASK)`) under `#ifdef __WXGTK__`.
  Needs a wxMSW/Win32 equivalent for pointer capture (wxMouseCaptureLost /
  `CaptureMouse` + warp, or Win32 raw input). GUI won't capture the mouse on
  Windows until this exists.
- [ ] **Keyboard scancodes** — `keyboard_x.c` is an **X11-keycode**→PS/2 Set-2
  table; `input_helpers.cpp:154-164` reads X11 raw codes via
  `GetRawKeyCode/Flags`. On wxMSW those return Windows VK/scancodes that **won't
  match** the X11 table, so it falls through to the portable `WXK_*` fallback
  (`input_helpers.cpp:7-150`) — **works for M1 with reduced fidelity**. Full parity
  (M4): add a Windows-scancode→PS/2 table (use the WM_KEYDOWN scancode bits) beside
  `keyboard_x.c`. VNC keysym mapping (`vnc_server.cpp:234`) is protocol-constant —
  fine.
- [ ] **Sound** — SDL2 (`plt_sound.cpp`, `SDL_OpenAudioDevice`/`SDL_QueueAudio`).
  Cross-platform, **no work** once SDL2 links. (ALSA is core-podule-only and
  optional-with-stubs — `src/CMakeLists.txt:129-145`.)
- [ ] **Paths** — `data_paths.cpp:19,43,57,228` hardcodes `/usr/share/rpcemu` and
  `getenv("HOME")` (no `USERPROFILE`); `headless_main.cpp:14-18,91,123` includes
  `<dirent.h>`/`<sys/stat.h>` and hardcodes the same. Use `wxStandardPaths` +
  `USERPROFILE` fallback.
- [ ] Linux-only CD-ROM init is already guarded `#if defined(__linux__)`
  (`emulator_host.cpp:37`) — compiles out cleanly on Windows.

### 6. Podules & CD-ROM  (M4, deferrable)

- [ ] Loadable podules — `podules.c:22,160-177` uses `dlopen`/`dlsym`/`dlclose` →
  `LoadLibrary`/`GetProcAddress`/`FreeLibrary`; hardcoded `.so` extension in the
  path template (`:157`) → `.dll`. Can ship with **built-in podules only** at first
  and defer loadable ones.
- [ ] CD-ROM — `cdrom-ioctl.c` is entirely Linux (`<linux/cdrom.h>`,
  `open("/dev/cdrom")`). **Use the portable `cdrom-iso.c`** (ISO-file backend) as
  the Windows default and skip the ioctl backend. A real-drive Windows backend
  (SPTI/IOCTL_CDROM) is optional future work.

### 7. Packaging & CI  (M5)

- [ ] CPack is DEB-only (`CMakeLists.txt:75-85`). Add an NSIS (installer) and/or
  ZIP generator for Windows.
- [ ] Install layout uses Unix FHS paths (`CMakeLists.txt:36-58`,
  `share/rpcemu`, `.desktop` at `src/gui/CMakeLists.txt:90-93`); guard the
  `.desktop` install and pick a Windows-appropriate layout (app dir + resources
  beside the `.exe`). `RPCEMU_INSTALL_DATADIR` baked in at
  `src/gui/CMakeLists.txt:54`.
- [ ] CI: `.github/workflows/build.yml` has only `linux-amd64` + `linux-arm64`,
  ELF smoke test at `:32`. Add a `windows-latest` job using **MSYS2**
  (`msys2/setup-msys2` action) + a direct `cmake` invocation (not `build.sh`), and
  a PE (`.exe`) smoke check.

---

## Out of scope

- **Bridged networking / TUN-TAP** (`network-tun.c`, `tap.h`). This is
  `/dev/net/tun` + Linux bridge ioctls + SIGIO async I/O (`:117-402`). The Windows
  equivalent is a signed TAP-Win32 kernel driver + overlapped I/O — a large,
  driver-dependent rewrite (the deleted `src/win/tap-win32.c` was 761 lines). NAT
  covers the overwhelming majority of users; defer unless bridged mode becomes a
  hard requirement. Note `tap.h` is already a dead interface header (no callers).
- **MSVC support.** MinGW-w64 only, per the toolchain decision.
- **Real-drive CD-ROM on Windows** — ISO-file backend only at first.

## Open questions

- MSYS2 `<dirent.h>` availability — if MinGW provides a usable `dirent`
  compat, the `opendir`/`readdir` sites (hostfs/romload/podulerom) may need little
  or no change. Confirm early; it materially shrinks §3.
- wxWidgets packaging: use the MSYS2 `wxwidgets3.2-msw` package vs a self-built
  wx. MSYS2 package is faster to start; a pinned self-build is more reproducible
  for releases.
- Whether to ship the MCP server / hostcmd / debugcmd tooling in the Windows
  package from day one (they work over TCP loopback once §4 lands) or defer.

## Reference — the deleted Qt5-era Windows layer

Recoverable from `git show 6828f03^:<path>` if useful as a starting point, but all
Qt5-era:
`src/win/rpc-win.c` (131 lines, disk info + OS logging),
`src/win/network-win.c` (244), `src/win/tap-win32.c` (761),
`src/win/cdrom-ioctl.c` (295), `src/hostfs-win.c` (214),
`src/qt5/keyboard_win.c` (157), plus `.rc`/`.ico`/`.manifest`/`RPCEmu.wxs`
(WiX installer).
