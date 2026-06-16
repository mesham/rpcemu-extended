# ARM dynamic recompiler (dynarec)

RPCEmu emulates the guest **ARM** CPU in two ways:

1. **Software interpreter** (`arm.c`) — decodes and executes each guest instruction in
   host code (`rpcemu-interpreter`).
2. **Dynamic recompiler** (`arm_dynarec.c` plus `codegen_*.c`) — translates guest ARM
   instruction sequences into native **x86** code and runs that (`rpcemu-recompiler`).

Both are implemented in C. The choice is **how guest instructions run**, not what
language the emulator is written in.

Even in a recompiler build, `arm_dynarec.c` still uses the same software interpreter
loop to compile new blocks and whenever execution must fall back (debugger, guest cache
off, no compiled block yet).

---

## Building

CMake option `RPCEMU_DYNAREC` (default **ON**) selects the CPU backend:

| `RPCEMU_DYNAREC` | CPU backend | Release binary |
| --- | --- | --- |
| ON | Dynarec (JIT) | `rpcemu-recompiler` |
| OFF | Software interpreter | `rpcemu-interpreter` |

Debug builds append `-debug` to the binary name.

The codegen backend follows the **host** CPU (`src/CMakeLists.txt`):

| Host | Codegen source |
| --- | --- |
| x86_64 | `codegen_amd64.c` |
| i386 / i686 | `codegen_x86.c` |
| Other (e.g. arm64) | Configure fails if dynarec is ON |

On arm64 Linux hosts, build the interpreter:

```bash
./build.sh --interpreter
# or: cmake -S . -B build -DRPCEMU_DYNAREC=OFF
```

See [COMPILE.md](../COMPILE.md) for the full build reference.

---

## Source layout

| File | Role |
| --- | --- |
| `src/arm.c` | Software interpreter (interpreter build only) |
| `src/arm_dynarec.c` | Execution loop, block compilation, debugger hooks |
| `src/arm_dynarec_ops.h` | Opcode dispatch table for the dynarec build |
| `src/codegen_amd64.c` / `codegen_x86.c` | Emit x86 machine code, block cache |
| `src/codegen_x86_common.h` | Shared helpers (i386 codegen) |
| `src/codegen_null.c` | Stubs when dynarec is disabled |

Compiled guest code is stored in `rcodeblock[][]`. On Linux, `set_memory_executable()`
in `arm_dynarec.c` marks that region with `mprotect(..., PROT_EXEC)`.

---

## How it works

### Block boundaries

While building a block (interpreter loop in `arm_exec()`), compilation stops at
branches, SWIs, coprocessor instructions, writes to R15, and some LDR/LDM forms that
reload PC. The same rules apply when falling back to software execution.

### Block cache

- `BLOCKS` = 1024 slots; each holds up to 1792 bytes of native code (~1.8 MB total).
- Guest PCs are hashed into `codeblockpc[]` (collisions possible).
- When the cache is full, slots are reused round-robin (`blockpoint` in codegen).

### Self-modifying code

Guest RAM writes go through `mem.c`, which calls `cacheclearpage()` so compiled code
for that page is dropped. Execution then recompiles as needed.

`resetcodeblocks()` clears the whole JIT cache on relevant **CP15** writes (for
example enabling the instruction cache, MMU/ROM/system control changes, TLB maintenance).

### When JIT vs interpreter runs

In a recompiler build, native code runs only when:

- Guest **data cache** is enabled (`dcache` from CP15 — toggled by RISC OS `*cache`,
  and exposed as `isblockvalid()` in codegen headers, which is effectively `dcache`),
- A compiled block exists for the current PC (hash slot matches),
- The debugger does not require per-instruction hooks.

With `*cache 0`, or when debugging (breakpoints, single-step, watchpoints, paused),
execution uses the software interpreter path inside `arm_dynarec.c` — the same loop
that compiles blocks. That improves compatibility; speed is usually close to
`rpcemu-interpreter`.

### Debugger

`debugger_requires_instruction_hook()` forces the interpreter path when the Machine
Inspector debugger is paused, stepping, or has breakpoints/watchpoints active.

---

## CPU models

Dynarec development and testing historically targeted **StrongARM (SA110)**. ARM610,
ARM710, and ARM7500 are selectable but less exercised.

Many programs that use self-modifying code work (e.g. Ovation, SparkFS). If something
misbehaves under JIT, try `rpcemu-interpreter` or `*cache 0` in a recompiler build.

FPA10 emulation is available with both CPU backends.

---

## i386 vs amd64 codegen

Both share the same driver in `arm_dynarec.c`. Differences:

| | i386 (`codegen_x86.c`) | amd64 (`codegen_amd64.c`) |
| --- | --- | --- |
| `BLOCKSTART` | 16 | 32 |
| Flag helpers | `LAHF` and lookup tables | x86-64 flag tests in emitted code |
| Block linking | Jump to next compiled block at block end | Same idea |

The amd64 backend is largely ported from i386. Some emitted code still relies on
32-bit absolute addresses; further tuning (register pinning, LDM/STM, chaining) is
possible.

---

## Limitations

- JIT requires an **x86** host. No dynarec when building for arm64.
- Block chaining (direct jump to the already-compiled successor block) helps loops;
  extending backward branches inside a single block was tried with little benefit.
- Guest prefetch and data aborts depend on correct interaction between generated loads,
  the MMU, and CP15 — problems here show up mainly on RISC OS 4.x.

---

## Origin

The dynarec was written by Sarah Walker for upstream RPCEmu. Spork Edition builds it
via CMake on Linux and ties it to the integrated Machine Inspector debugger as
described above.
