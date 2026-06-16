# How to Compile RPCEmu

RPCEmu (Spork Edition) is a **Linux-only** build. It uses **CMake** and the `build.sh`
script at the project root.

The emulator core (`src/`) is C11. The wxWidgets front-end (`src/gui/`) is C++17.

---

## Quick start

```bash
./setup-build-env.sh    # once: install apt dependencies
./build.sh --zip        # release build + staged tarball
```

Run the staged binary:

```bash
./releases/linux/amd64/rpcemu-recompiler
```

Or run directly from the build tree (requires runtime data in the project root):

```bash
./build/bin/rpcemu-recompiler
```

Debian packages install the binary to `/usr/bin`, bundled assets under
`/usr/share/rpcemu/`, and per-user data under `~/.local/share/rpcemu/` (or
`$XDG_DATA_HOME/rpcemu/`). On first run, default configs and machine templates
are copied into the user data directory. Override paths with `RPCEMU_DATADIR` or
`RPCEMU_RESOURCE_DIR` if needed.

**Important:** run RPCEmu from the project root (or from a staged release directory)
so it can find `configs/`, `machines/`, `roms/`, `resources/`, `poduleroms/`, and
`shared/` when using portable tarballs. Installed `.deb` packages resolve these
automatically.

---

## Build script reference

```bash
./build.sh                         # Linux release build (dynarec)
./build.sh --interpreter           # interpreter build (no dynarec)
./build.sh --debug                 # debug build (-debug suffix on binary name)
./build.sh --arch arm64            # Linux arm64 (native on Pi)
./build.sh --cross-arm64           # cross-compile Linux arm64 from x86_64
./build.sh --deb                   # Linux + .deb for selected arch
./build.sh --zip                   # Linux .tar.gz in releases/linux/
./build.sh --podules               # rebuild HostFS podule ROMs
./build.sh --clean                 # remove build trees and releases/
./build.sh --help                  # full option list
```

### Output locations

| Build | Binary | Staged release |
| --- | --- | --- |
| Linux dynarec (default) | `build/bin/rpcemu-recompiler` | `releases/linux/<arch>/rpcemu-recompiler` |
| Linux interpreter | `build/bin/rpcemu-interpreter` | `releases/linux/<arch>/rpcemu-interpreter` |
| Linux debug | `build/bin/rpcemu-recompiler-debug` | `releases/linux/<arch>/rpcemu-recompiler-debug` |
| Linux `.deb` | — | `releases/linux/<arch>/rpcemu_<version>_<arch>.deb` |

Supported Linux architectures: **amd64** and **arm64** (`--arch` or native host).

Release archive: `releases/linux/rpcemu_<version>_linux_<arch>.tar.gz`

The Linux release staging step copies `configs/`, `poduleroms/`, `resources/`,
`roms/roms.txt`, and a default `machines/Default/` tree. The `shared/` directory is
created automatically at runtime if it does not exist.

---

## Dependencies

Installed automatically by `./setup-build-env.sh` on Debian/Ubuntu:

| Package | Purpose |
| --- | --- |
| `build-essential`, `cmake`, `pkg-config` | Build toolchain |
| `libwxgtk3.2-dev` | wxWidgets GUI (GTK 3 backend) |
| `libsdl2-dev` | Audio output |
| `libvncserver-dev` | Built-in VNC server |
| `libgs-dev`, `ghostscript` | In-process PostScript → PDF conversion (optional but recommended) |

Optional extras:

```bash
./setup-build-env.sh --cross-arm64  # aarch64-linux-gnu (cross from x86 PC)
./setup-build-env.sh --podules      # ARM binutils for podule ROM rebuild
```

---

## Manual CMake

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DRPCEMU_DYNAREC=ON \
  -DRPCEMU_ENABLE_VNC=ON \
  -DRPCEMU_ENABLE_GHOSTPDL=ON

cmake --build build -j"$(nproc)"
./build/bin/rpcemu-recompiler
```

### CMake options

| Option | Default | Description |
| --- | --- | --- |
| `RPCEMU_DYNAREC` | ON | Link the ARM-to-x86 dynarec (x86 hosts only) |
| `RPCEMU_ENABLE_VNC` | ON | VNC server support (requires libvncserver) |
| `RPCEMU_ENABLE_GHOSTPDL` | ON | Link against Ghostscript/GhostPDL for in-process PDF conversion |
| `RPCEMU_ENABLE_WARNINGS` | ON | Extra compiler warnings (`-Wall -Wextra -Werror=switch`) |

Interpreter build:

```bash
cmake -S . -B build -DRPCEMU_DYNAREC=OFF
cmake --build build -j"$(nproc)"
```

Debug build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j"$(nproc)"
```

On **arm64** hosts (e.g. Raspberry Pi), use `--interpreter` or `-DRPCEMU_DYNAREC=OFF`
because the dynarec JIT targets x86 hosts.

Further detail: [docs/dynarec.md](docs/dynarec.md).

---

## GhostPDL / Ghostscript (in-process PDF conversion)

When `libgs-dev` is present at configure time, RPCEmu links against `libgs` and can
convert captured PostScript `.prn` files to PDF without external tools. Enable **Also
create PDF files** in *Settings → Parallel Port*.

For PCL/XPS print jobs, install full GhostPDL and point the build at it:

```bash
export GHOSTPDL_PREFIX=/opt/ghostpdl
./build.sh
```

---

## Rebuilding podule ROMs

```bash
./setup-build-env.sh --podules
./build.sh --podules
```

Requires `arm-linux-gnueabi-as` and related binutils.

---

## Continuous integration and releases

GitHub Actions builds on every push/PR to `main`. Push a version tag (e.g. `v1.0.0`)
to publish a GitHub Release with the Linux tarball. Update `VERSION` before tagging.

---

## Troubleshooting builds

| Problem | Remedy |
| --- | --- |
| `cmake not found` | Run `./setup-build-env.sh` |
| wxWidgets not found | Install `libwxgtk3.2-dev` |
| VNC build fails | Install `libvncserver-dev`, or `-DRPCEMU_ENABLE_VNC=OFF` |
| Ghostscript not detected | Install `libgs-dev`, or `-DRPCEMU_ENABLE_GHOSTPDL=OFF` |
| Dynarec fails on arm64 | Use `./build.sh --interpreter` |

---

## Runtime troubleshooting

| Problem | Remedy |
| --- | --- |
| Emulator cannot find configs/ROMs | Run from the project root or a staged release directory |
| No audio | Ensure PulseAudio or PipeWire is running (SDL2) |
| No network | Confirm NAT is selected in machine settings; SLiRP is always enabled on Linux |
| No VNC menu item | Rebuild with `libvncserver-dev` installed |
| Log file | Check `rpclog.txt` in the data directory |
