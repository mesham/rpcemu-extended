# Building RPCEmu (Spork Edition) for macOS

macOS builds produce a **universal binary** (`rpcemu`) containing two slices:

| Slice    | Engine                       | Why                                                            |
|----------|------------------------------|----------------------------------------------------------------|
| `x86_64` | recompiler (dynarec)         | The JIT (`codegen_amd64.c`) emits x86-64 machine code.         |
| `arm64`  | interpreter                  | The native arm64 recompiler ([arm64-dynarec.md](arm64-dynarec.md)) is new and not yet enabled for macOS release builds. |

The two slices are built **separately** and fused with `lipo`. On Apple Silicon
the x86_64 slice also runs (fast) under Rosetta 2, so an x86_64-only download is a
reasonable alternative to universal. A hardened, notarised arm64 build using the
native recompiler would additionally need the `MAP_JIT` handling noted under
[Limitations](#limitations).

## Recommended: build on macOS (GitHub Actions)

The CI workflow (`.github/workflows/build.yml`) builds macOS natively on
GitHub's runners — the only place these builds get *executed and tested*:

- `macos-x86_64` (Intel runner): x86_64 + dynarec, runs the JIT unit test.
- `macos-arm64` (Apple Silicon runner): arm64 + interpreter, runs the test.
- `macos-universal`: `lipo`s the two slices, stages `releases/macos/universal/`.

Locally on a Mac you can reproduce this with:

```sh
brew install cmake ninja pkg-config wxwidgets sdl2 libvncserver
./build-macos.sh            # both slices + lipo -> releases/macos/universal/
```

## Cross-compiling from Linux (osxcross)

For fast compile-iteration only: a Linux host **cannot run** the resulting
Mach-O binaries, so real verification is the native/CI build above.

It also **requires a macOS SDK you provide yourself** — it cannot be downloaded
(it comes from Xcode, behind an Apple ID). Produce a `MacOSX<NN>.sdk.tar.xz`
(see the osxcross README, "packaging the SDK"; needs SDK ≥ 11 for arm64) and:

```sh
mkdir -p macos-sdk && cp /path/to/MacOSX14.sdk.tar.xz macos-sdk/
./setup-macos-cross-build-env.sh     # builds osxcross + cross wxWidgets/SDL2
source ./macos-cross-env.sh
./build-macos.sh                     # cross-builds both slices + lipo
```

The cross build drops VNC and Ghostscript (extra cross-built libraries not
worth the effort for the unverified path); the native/CI build keeps them.

## Notable macOS-specific code

- `src/arm_dynarec.c` — `set_memory_executable()` uses POSIX `mprotect(RWX)` on
  the static JIT buffer (works on an unsigned/ad-hoc Intel build). A hardened
  runtime / notarised build would need an `mmap(MAP_JIT)` buffer plus
  `pthread_jit_write_protect_np()`; deferred until we sign for distribution.
- `src/socket-compat.h` — macOS has no `MSG_NOSIGNAL`; SIGPIPE is suppressed
  per-socket via `SO_NOSIGPIPE` instead.
- `src/CMakeLists.txt` — macOS uses `network-null.c` + the portable ISO CD-ROM
  backend (like Windows); the Linux `/dev/net/tun` bridge and CD-ROM ioctl
  backends are not built. NAT networking (SLiRP) still works.

## Limitations

- The release is a **portable folder** (the `rpcemu` binary plus its resource
  directories), not a signed, notarised `.app` bundle — that would need a Mac and
  an Apple Developer account.
- The dynarec buffer is made executable with plain `mprotect(RWX)`, which works
  for an unsigned/ad-hoc build. A hardened, notarised build would need a
  `MAP_JIT` buffer with `pthread_jit_write_protect_np()`.
