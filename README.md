# RPCEmu – Spork Edition

An extended fork of **[RPCEmu](http://www.marutan.net/rpcemu/)**, the open-source
emulator for Acorn Risc PC and A7000 machines. This edition targets **Linux** with a
wxWidgets front-end, multi-machine configuration, integrated debugger and machine
inspector, Access/ShareFS networking, full FPA emulation, and modern CMake-based
build tooling.

Licensed under the **GNU GPL v2** — see `COPYING`.

---

## Highlights

- **Multi-machine configuration** — create, edit, clone, and delete machine profiles from a startup selector; each machine has isolated CMOS, HostFS, and hard disc storage.
- **Quick machine switching** — switch between machines via *File → Recent Machines* without restarting.
- **Dual HostFS drives** — per-machine **HostFS** plus a common **Shared** drive (`shared/`) visible to all machines.
- **Access/ShareFS networking** — NAT-mode relay for Acorn Access and ShareFS file sharing between emulated and real machines.
- **Expansion cards (podules)** — assign emulated podules per machine (*Settings → Machine → Podules*): ROM, MIDI (AKA16/AKA12/MIDI Max, host MIDI via ALSA), and the Computer Concepts Lark sampler. Plugin ABI for adding more. See [docs/podules.md](docs/podules.md).
- **Full FPA10 emulation** — floating-point coprocessor with cycle-accurate timing; works with interpreter and dynarec.
- **Pixel Perfect scaling** — optional integer scaling for sharp pixels (*Settings → Pixel Perfect*).
- **Built-in VNC server** — remote desktop access from any VNC client.
- **Headless mode** — run a machine with no GUI window, accessed entirely over VNC (`--headless --machine <name>`). Genuinely display-less: needs no X11/Wayland, so it runs on a headless server. See [Headless mode](#headless-mode).
- **HostCmd — drive the RISC OS command line from the host** — run guest commands from the host over a local socket and stream their output back, with the return code. Edit on the host (via HostFS), compile on the guest (`rpcemu-run -- cc -c hello`), or open an interactive RISC OS shell (`rpcemu-shell`). Ideal for IDE/LLM-driven development. See [docs/hostcmd.md](docs/hostcmd.md).
- **MCP server — drive RISC OS from Claude / an agent** — a [Model Context Protocol](https://modelcontextprotocol.io) server exposing tools to run guest commands, read/write/list files (via HostFS), capture and click the screen, and inspect/control the emulated ARM CPU (registers, memory, disassembly, breakpoints, watchpoints, single-step). Point Claude Code / Desktop at it for agent-driven RISC OS development. Setup and tool reference in [tools/mcp/README.md](tools/mcp/README.md).
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
| `poduleroms/` | Compiled extension ROM images (HostFS, ScrollWheel — the built-in Support podule) |
| `podules/` | Expansion-card (podule) ROMs — shipped system components, selectable per machine |
| `riscos-progs/` | RISC OS module source (HostFS, HostFSFiler, ScrollWheel, EtherRPCEm) |
| `packaging/` | Desktop entry and other packaging files |
| `build.sh` | Unified build and release script |
| `docs/dynarec.md` | ARM dynamic recompiler (build, behaviour, limitations) |
| `docs/peripherals.md` | Serial and parallel ports (file logging, TCP modem, printer) |
| `docs/podules.md` | Expansion cards (podules): bundled devices, configuration, plugin ABI |
| `docs/hostcmd.md` | HostCmd: drive the RISC OS command line from the host (`rpcemu-run`/`rpcemu-shell`) |
| `tools/mcp/README.md` | MCP server: drive a RISC OS machine from Claude / an agent (commands, files, screen, debugger). Setup + tool reference. |
| `docs/debugcmd.md` | DebugCmd: control the emulated CPU over a socket (registers, memory, disassembly, breakpoints, single-step) |
| `setup-build-env.sh` | Install build dependencies (Debian/Ubuntu) |

### Where your data lives

When **installed** (e.g. from the `.deb`), the binary and read-only support files
(ROMs, podule ROMs, templates) live under `/usr/share/rpcemu`, while your own
machines, configs, ROMs, HostFS and logs are kept in a visible **`~/RPCEmu/`** folder,
seeded from the shared templates on first run. An existing `~/.local/share/rpcemu` from
an earlier version is migrated automatically. The **portable** `.tar.gz` instead keeps
everything self-contained in its own folder.

---

## Getting started

### Supported systems

The prebuilt releases are **amd64** (x86, dynarec) and **arm64** (e.g. Raspberry Pi —
interpreter build, as there is no ARM dynarec yet) Linux packages, built on **Ubuntu
24.04 LTS**. Because they are dynamically linked, they run on distributions whose system
libraries are that version or newer. In practice that means:

| Distribution | Runs the prebuilt release? |
| --- | --- |
| Ubuntu 24.04 LTS (Noble) and newer (24.10, 25.04, …) | ✅ Yes — primary target |
| Linux Mint 22 / 22.x, Pop!_OS 24.04, Zorin 18, elementary 8, KDE neon (24.04 base) | ✅ Yes |
| Debian 13 (Trixie) and newer | ✅ Yes |
| arm64 / Raspberry Pi (Ubuntu 24.04+ base) | ✅ Yes — `arm64` package (interpreter; slower than x86) |
| Ubuntu 22.04 LTS, Debian 12 (Bookworm) and older | ❌ No — system libraries too old |

Minimum requirements: **glibc ≥ 2.34**, **libstdc++ from GCC 13.2+** (`GLIBCXX_3.4.32`),
and **wxWidgets 3.2**. These ship as standard on Ubuntu 24.04-era distributions.

On an older or different distribution (or for arm64), **build from source** instead —
`./setup-build-env.sh` then `./build.sh` adapts to your system's libraries. See
[COMPILE.md](COMPILE.md).

### Install the `.deb`

Install with **apt** — not `dpkg -i`, which reports missing dependencies but won't fetch
them. The runtime libraries (wxWidgets, SDL2, libvncserver, …) live in Ubuntu's
**`universe`** component, so make sure it's enabled first:

```bash
sudo add-apt-repository universe     # if not already enabled
sudo apt update
sudo apt install ./rpcemu_*_amd64.deb   # or _arm64.deb on a Pi
```

`apt` reads the package's declared dependencies and pulls them in. If `apt` complains the
packages are *"not installable"*, it's almost always because `universe` isn't enabled or
the package lists are stale — the two commands above fix that.

The portable `.tar.gz` instead bundles everything in one folder; run
`./setup-runtime-env.sh` once to install its runtime libraries. See [Run](#run) below.

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

If you downloaded the portable **`.tar.gz`** release and see an error like
`error while loading shared libraries: libwx_gtk3u_core-3.2.so.0`, install the
runtime libraries once:

```bash
./setup-runtime-env.sh
```

(The **`.deb`** package pulls these in automatically via apt, so this step is only
needed for the portable tarball.)

### First launch

1. The **Machine Selector** dialog lists available configurations.
2. Use **New**, **Edit**, **Clone**, or **Delete** to manage machines.
3. Select a machine and click **Start**.
4. Place licensed RISC OS ROM files in `roms/<subdir>/` and select the ROM folder in
   the machine editor.

### Headless mode

A machine can be run without the GUI window and accessed entirely over the
built-in VNC server — useful for servers or always-on machines:

```bash
./rpcemu-recompiler --headless --machine <name>
```

- `--machine <name>` selects a machine by its config name (the file in `configs/`,
  with or without the `.cfg` suffix). It is required in headless mode, since there
  is no interactive selector.
- `--list-machines` prints the available machine names and exits.
- `--help` (or `-h`) prints usage and exits. All three of these run without a display.
- The chosen machine **must have the VNC server enabled** (`vnc_enabled=1`) in its
  configuration; headless mode refuses to start otherwise, as there would be no way
  to reach the machine. The VNC port/password come from that same config.
- Press **Ctrl-C** (or send `SIGTERM`) to shut down cleanly — CMOS, disc images, and
  configuration are saved on exit, just as when closing the GUI window.

Headless mode is genuinely display-less: it is handled before any GUI toolkit is
initialised, so it needs **no X11/Wayland display** and runs on a server with no
desktop installed. Data is located via `$RPCEMU_DATADIR`, else the executable or
current directory if it contains a `configs/` folder, else the install prefix.

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
- Headless mode for display-less servers (run a machine over VNC with no GUI)
- HostCmd: drive the guest RISC OS command line from the host (`rpcemu-run`/`rpcemu-shell`) for edit-on-host/compile-on-guest workflows
- MCP server for agent-driven RISC OS development: run commands, edit/build, screenshot, and inspect/control the emulated CPU (see `tools/mcp/`)
- Virtual printer with optional Ghostscript PDF conversion
- Serial log-to-file and a real telnet TCP modem (dial BBSes, 8-bit-clean transfers)
- Machine Inspector with disassembly and memory browser
- Dynarec debugger hooks for consistent breakpoint/watchpoint behaviour
- Robustness & memory-safety hardening: bounds-checked HFE/ADF disc-image and HostFS input handling, FPA faults raised as undefined instructions rather than aborting the emulator, and a fixed use-after-free on GUI shutdown
- CMake build system for Linux (amd64 and arm64)

---

## Troubleshooting

| Symptom | Remedy |
| --- | --- |
| `error while loading shared libraries: …` (tarball) | Run `./setup-runtime-env.sh` to install the runtime libraries (wxWidgets, SDL2, libvncserver, Ghostscript) |
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
