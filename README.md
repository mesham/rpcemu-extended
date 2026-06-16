# RPCEmu – Spork Edition

An extended fork of **[RPCEmu](http://www.marutan.net/rpcemu/)**, the open-source
emulator for Acorn Risc PC and A7000 machines. This edition targets **Linux** with a
wxWidgets front-end, multi-machine configuration, integrated debugger and machine
inspector, Access/ShareFS networking, full FPA emulation, and modern CMake-based
build tooling.

Licensed under the **GNU GPL v2** — see `COPYING`.

## Screenshots

![Machine Selection](screenshots/machine-selection.png)
![Main Window](screenshots/new-layout.png)
![VNC Server](screenshots/vnc.png)
![Access Networking](screenshots/access.png)
![Machine Inspector](screenshots/inspector.png)
![Disassembly View](screenshots/diss.png)
![Memory Browser](screenshots/mem.png)

---

## Highlights

- **Multi-machine configuration** — create, edit, clone, and delete machine profiles from a startup selector; each machine has isolated CMOS, HostFS, and hard disc storage.
- **Quick machine switching** — switch between machines via *File → Recent Machines* without restarting.
- **Dual HostFS drives** — per-machine **HostFS** plus a common **Shared** drive (`shared/`) visible to all machines.
- **Access/ShareFS networking** — NAT-mode relay for Acorn Access and ShareFS file sharing between emulated and real machines.
- **Full FPA10 emulation** — floating-point coprocessor with cycle-accurate timing; works with interpreter and dynarec.
- **Pixel Perfect scaling** — optional integer scaling for sharp pixels (*Settings → Pixel Perfect*).
- **Built-in VNC server** — remote desktop access from any VNC client.
- **Parallel port** — log raw output to a file, or a virtual printer that captures jobs to `.prn` files with optional in-process PDF conversion via Ghostscript.
- **Serial port** — log to file, or a TCP "modem" that dials real telnet BBSes (`ATDT host:port`) with a telnet client layer and 8-bit-clean X/Y/ZMODEM transfers. See [docs/peripherals.md](docs/peripherals.md).
- **Machine Inspector** — live CPU, disassembly, memory, peripheral, and debugger views with auto-refresh.
- **Integrated debugger** — pause/resume, single-step, breakpoints, and watchpoints; dynarec-aware via shared hooks.
- **Toolbar and status bar** — quick access to common actions; activity indicators for floppy, IDE, HostFS, and network.
- **Recent disc images** — quick access to recently used floppy and CD-ROM images.

---

## Architecture

The codebase splits into two layers:

| Layer | Path | Language | Role |
| --- | --- | --- | --- |
| **Core** | `src/` | C11 | Guest ARM CPU (interpreter or dynarec), devices, SLiRP, debugger |
| **GUI** | `src/gui/` | C++17 | wxWidgets front-end, threading bridge, dialogs, VNC server |

The GUI runs emulation on a **worker thread** (`EmulatorHost`). UI events are posted
as commands; video updates and debugger notifications come back through a `GuiBridge`
interface. Inspector snapshots are marshalled off the emulator thread as plain
`MachineSnapshot` structs.

Build with **CMake** — see [COMPILE.md](COMPILE.md) for full details.

---

## Project layout

| Path | Purpose |
| --- | --- |
| `src/` | Emulator core (CPU, VIDC, IOMD, IDE, FDC, FPA, HostFS, SLiRP, …) |
| `src/gui/` | wxWidgets front-end, machine inspector, configuration dialogs |
| `configs/` | Machine configuration files (`.cfg`, INI format) |
| `machines/<name>/` | Per-machine runtime data: `cmos.ram`, `hostfs/`, `hd4.hdf`, `hd5.hdf` |
| `shared/` | Common folder exposed as `HostFS::Shared.$` (created at startup if missing) |
| `roms/` | RISC OS ROM images — see [official ROM instructions](http://www.marutan.net/rpcemu/manual/romimage.html) |
| `resources/` | Blank floppy/disc templates for *Disc → Floppy → Create Blank* |
| `poduleroms/` | Compiled podule ROM images |
| `riscos-progs/` | RISC OS module source (HostFS, HostFSFiler, ScrollWheel, EtherRPCEm) |
| `packaging/` | Desktop entry and other packaging files |
| `build.sh` | Unified build and release script |
| `docs/dynarec.md` | ARM dynamic recompiler (build, behaviour, limitations) |
| `docs/peripherals.md` | Serial and parallel ports (file logging, TCP modem, printer) |
| `setup-build-env.sh` | Install build dependencies (Debian/Ubuntu) |

---

## Getting started

### Build

```bash
./setup-build-env.sh    # install dependencies (Debian/Ubuntu)
./build.sh --zip        # build and package to releases/linux/amd64/
./build.sh --deb --zip  # + .deb package
```

See [COMPILE.md](COMPILE.md) for manual CMake, GhostPDL, and podule ROM rebuilds.

### Run

```bash
./releases/linux/amd64/rpcemu-recompiler
```

Run from the project root (or a staged release directory) so data files are found.

### First launch

1. The **Machine Selector** dialog lists available configurations.
2. Use **New**, **Edit**, **Clone**, or **Delete** to manage machines.
3. Select a machine and click **Start**.
4. Place licensed RISC OS ROM files in `roms/<subdir>/` and select the ROM folder in
   the machine editor.

---

## Machine configuration

Each machine is defined by a `.cfg` file in `configs/` and a data directory under
`machines/<name>/`.

| Setting | Options |
| --- | --- |
| **Model** | RiscPC ARM610/710/810/StrongARM, A7000, A7000+ (experimental), Phoebe (experimental) |
| **RAM** | 4, 8, 16, 32, 64, 128, or 256 MB |
| **VRAM** | None or 2 MB |
| **ROM** | Subdirectory under `roms/` containing ROM components |
| **Refresh rate** | 20–100 Hz |
| **Network** | Off, NAT, Ethernet Bridging, IP Tunnelling |
| **Hard discs** | HardDisc 4 and 5 — create 256 MB, 512 MB, 1 GB, or 2 GB images |

Configuration keys are stored under a `[General]` group (wxFileConfig INI format).
NAT port-forward rules are stored in a separate `[nat_port_forward_rules]` group.

---

## HostFS and Shared drives

Two filing system icons appear on the RISC OS icon bar:

| Icon | RISC OS path | Host path | Scope |
| --- | --- | --- | --- |
| **HostFS** | `HostFS::HostFS.$` | `machines/<name>/hostfs/` | Per-machine |
| **Shared** | `HostFS::Shared.$` | `shared/` | All machines |

Use HostFS for machine-specific files and Shared for utilities or files you want
available across configurations.

---

## Machine Inspector and debugger

Open **Debug → Machine Inspector…** (or use the toolbar button).

| Tab | Contents |
| --- | --- |
| **CPU** | Registers R0–R15, CPSR, mode, MMU state, dynarec/interpreter, performance |
| **Disassembly** | ARM disassembly at a chosen address, optional follow-PC |
| **Memory** | Hex dump of emulated memory at a chosen address |
| **Debugger** | Run/Pause/Step, breakpoint and watchpoint lists, last halt reason |
| **Peripherals** | VIDC, IOMD IRQ/timers, floppy, IDE, podule slot summary |

Auto-refresh runs every 500 ms by default. Breakpoints and watchpoints work while
the dynarec is active — `arm_dynarec.c` checks `debugger_requires_instruction_hook()`
before executing translated blocks.

---

## Serial and parallel ports

Configure via **Settings → Serial…** and **Settings → Parallel…**. The Risc PC has a
single hardware serial port (the 16550 UART at `0x3F8`), so only one **Serial** port
is exposed.

| Port | Modes |
| --- | --- |
| **Serial (0x3F8)** | Disabled, log to file, TCP modem (telnet) |
| **Parallel (LPT)** | Disabled, log to file, virtual printer |

- **Log to file** captures the raw byte stream the guest sends — handy for debugging
  or capturing print/serial output.
- **TCP modem** answers the Hayes AT command set and `ATDT host:port` opens a real TCP
  connection. It speaks telnet and negotiates binary mode, so telnet BBSes work and
  X/Y/ZMODEM transfers stay 8-bit clean. `+++` (guard-timed) returns to command mode;
  `ATH` hangs up.
- **Virtual printer** writes `.prn` files to a chosen folder
  (default: `machines/<name>/printjobs/`); with Ghostscript support, enable **Also
  create PDF files** for automatic conversion.

Physical host passthrough (`/dev/tty*`) is not yet implemented. Full details,
including AT commands and how RISC OS drives each port, are in
[docs/peripherals.md](docs/peripherals.md).

---

## Keyboard and host controls

RPCEmu does **not** bind any host keyboard shortcuts, so every key — including the
function keys (**F12** for the RISC OS command line, etc.) and Ctrl combinations —
passes straight through to RISC OS. All emulator actions (screenshot, reset, floppy
load/eject, full-screen, mute, machine settings, and the debugger Run/Pause/Step
controls) are available from the menus and the toolbar instead.

| Key | Action |
| --- | --- |
| **Ctrl+End** | Release the captured mouse, or exit full-screen |

The toolbar provides one-click access to screenshot, floppy load, CD-ROM ISO load,
reset, mute, full-screen, machine settings, and debugger controls.

---

## FPA (Floating Point Accelerator) emulation

Complete FPA10 coprocessor emulation in `src/fpa.c`:

- **Dyadic:** ADF, MUF, SUF, RSF, DVF, RDF, POW, RPW, RMF, FML, FDV, FRD, POL
- **Monadic:** MVF, MNF, ABS, RND, SQT, LOG, LGN, EXP, SIN, COS, TAN, ASN, ACS, ATN, URD, NRM
- **Conversion:** FIX, FLT (all IEEE rounding modes)
- **Comparison:** CMF, CMFE, CNF, CNFE with NaN handling
- **Transfer:** LDF, STF, LFM, SFM

Cycle costs are modelled (e.g. 10 cycles for load/store, 150 for SIN/COS/TAN).
Works with both interpreter and dynarec. See [docs/dynarec.md](docs/dynarec.md) for
how the JIT is built and when it falls back to interpretation.

---

## Differences from upstream RPCEmu

- wxWidgets front-end with machine selector, toolbar, and integrated debugger
- Multi-machine configuration with isolated per-machine storage
- Quick machine switching and recent-machines menu
- Dual HostFS drives (per-machine + shared)
- Access/ShareFS broadcast relay for NAT networking
- Full FPA10 emulation with cycle timing
- Pixel Perfect integer scaling
- Built-in VNC server
- Virtual printer with optional Ghostscript PDF conversion
- Serial log-to-file and a real telnet TCP modem (dial BBSes, 8-bit-clean transfers)
- Machine Inspector with disassembly and memory browser
- Dynarec debugger hooks for consistent breakpoint/watchpoint behaviour
- CMake build system for Linux (amd64 and arm64)

---

## Troubleshooting

| Symptom | Remedy |
| --- | --- |
| Window does not appear / configs not found | Run from the project root or a staged release directory |
| No audio | Ensure PulseAudio or PipeWire is running (SDL2) |
| No network | Select NAT in machine settings; SLiRP is always compiled in on Linux |
| ROM not found | Place ROM files in `roms/<subdir>/` and select the folder in machine settings |
| Machine data not persisting | Check that `machines/<name>/` exists and is writable |
| VNC option missing | Rebuild with `libvncserver-dev` installed |
| PDF conversion unavailable | Install `libgs-dev` and rebuild; runtime needs Ghostscript resource files |
| Diagnostic log | See `rpclog.txt` in the data directory |

---

## Contributing

Issues and pull requests are welcome, especially around debugger, inspector, and
networking features.

---

## License and credits

- Licensed under the **GNU General Public License v2**. See `COPYING`.
- Original emulator by the RPCEmu contributors.
- Spork Edition enhancements by Andrew Timmins and contributors.
