# Debugger: exception trapping, SWI tracing, logging watchpoints

Spec for three additions to RPCEmu's debugger, bringing it ahead of Arculator's
on every axis. All three are implemented in C in `rpcemu.c` plus a handful of
call-site hooks; **no code-generator changes are required**.

## Enabling facts (verified)

Both cores funnel CPU exceptions through an `exception(mmode, address, diff)`
function and every SWI through `opSWI()`. There are **two** copies of
`exception()` — interpreter `arm.c:483` and dynarec `arm_dynarec.c:495` — so each
needs the hook. `opSWI()` (`arm_common.c:570`) is shared by both cores.

Exception classification, from the actual call sites:

| `mmode` | `address` | Meaning              | Trap? |
|---------|-----------|----------------------|-------|
| UNDEFINED (11) | 0x08 | Undefined instruction | yes |
| SUPERVISOR (3) | 0x0c | SWI                   | via SWI hook, not exception hook |
| ABORT (7)      | 0x10 | Prefetch abort        | yes |
| ABORT (7)      | 0x14 | Data abort            | yes |
| IRQ (2)        | 0x1c | IRQ                   | no |
| FIQ (1)        | 0x20 | FIQ                   | no |

A distinct 26-bit "address exception" is not separately raised by this core, so
the trap set is: **undefined instruction, prefetch abort, data abort**.

## Data model (`rpcemu.h`)

Extend `DebugPauseReason`:
```c
DebugPauseReason_Exception = 5,
DebugPauseReason_Swi       = 6,
```

`DebugWatchpointInfo.reserved0` is repurposed as `uint8_t log_only` — no struct
size / snapshot-ABI change.

New PODs (shared by core + GUI):
```c
typedef enum {
    TraceEvent_Exception = 0,
    TraceEvent_Swi       = 1,
    TraceEvent_Watchpoint = 2,
} TraceEventType;

typedef struct DebugTraceEvent {
    uint32_t seq;     /* monotonic; GUI detects drops via gaps */
    uint32_t type;    /* TraceEventType */
    uint32_t pc;      /* faulting / calling PC */
    uint32_t opcode;  /* instruction word (0 if N/A) */
    uint32_t arg0;    /* exc: exception kind | swi: number   | wp: address     */
    uint32_t arg1;    /* exc: abort address  | swi: R0        | wp: value       */
    uint32_t arg2;    /* swi: cpsr flags     | wp: size<<1|is_write             */
} DebugTraceEvent;

typedef struct DebugTraceConfig {
    uint8_t trap_undefined;
    uint8_t trap_prefetch_abort;
    uint8_t trap_data_abort;
    uint8_t log_exceptions;     /* also emit exception events to the ring */
    uint8_t swi_trace_enabled;  /* emit SWI events to the ring */
    uint8_t swi_trace_halt;     /* halt on a matching SWI */
    uint8_t reserved0;
    uint8_t reserved1;
    uint32_t swi_filter_min;    /* inclusive SWI-number range; 0..0xffffffff = all */
    uint32_t swi_filter_max;
} DebugTraceConfig;
```

## Core (`rpcemu.c`)

Single-writer ring (emulator thread only writes during execution and drains
during command processing — both on the emulator thread, so no locking):
```c
#define DEBUGGER_TRACE_RING_SIZE 4096  /* power of two */
static DebugTraceEvent debugger_trace_ring[DEBUGGER_TRACE_RING_SIZE];
static uint32_t debugger_trace_head, debugger_trace_tail;
static uint32_t debugger_trace_dropped, debugger_trace_seq;
static DebugTraceConfig debugger_trace_config;
int debugger_swi_trace_active; /* fast gate, mirrors swi_trace_enabled|halt */
```

New API:
```c
void debugger_set_trace_config(const DebugTraceConfig *cfg);
void debugger_get_trace_config(DebugTraceConfig *cfg);
uint32_t debugger_drain_trace_events(DebugTraceEvent *out, uint32_t max, uint32_t *dropped);
void debugger_exception_hook(uint32_t mmode, uint32_t address, uint32_t pc); /* called from both exception() copies */
int  debugger_swi_hook(uint32_t swinum, uint32_t opcode);                    /* called from opSWI(); 1 => halt */
```

- `debugger_exception_hook`: classify; if `log_exceptions`, push event; if the
  matching `trap_*` flag is set, `debugger_enter_pause(DebugPauseReason_Exception,
  pc, opcode)`. `exception()` then completes normally and the core halts at the
  vector handler's first instruction via the existing `debugger_instruction_hook`.
- `debugger_swi_hook`: range-filter; push event; if `swi_trace_halt`,
  `debugger_enter_pause(DebugPauseReason_Swi, ...)` and return 1.
- `debugger_memory_access` (existing, `rpcemu.c:462`): on a match, if
  `wp->log_only` push a `TraceEvent_Watchpoint` and `continue` instead of pausing.
- `debugger_requires_instruction_hook` (existing, `rpcemu.c:289`): also return 1
  when any `trap_*` or `swi_trace_halt` is set, so the halting path is active.

## Hook call sites
- `arm.c:483` — top of `exception()`: `debugger_exception_hook(mmode, address, arm.reg[15]);`
- `arm_dynarec.c:495` — top of `exception()`: same.
- `arm_common.c:572` — after `swinum` extraction, gated:
  `if (debugger_swi_trace_active && debugger_swi_hook(swinum, opcode)) return 0;`

## Delivery (`gui/emulator_host.*`)

New commands alongside the existing debugger commands (`emulator_host.cpp:643`):
- `SetDebugTraceConfig` — pushes a `DebugTraceConfig` to the core.
- `DrainTraceEvents` — synchronous (like `TakeSnapshot`, `emulator_host.cpp:913`);
  copies up to N events + dropped-count out under the snapshot mutex/cv. GUI calls
  it on a ~100 ms timer while tracing is active.

Snapshot (`machine_snapshot.h`) gains status-only fields:
`DebugTraceConfig debug_trace_config; uint32_t debug_trace_dropped, debug_trace_pending;`

## GUI (`gui/machine_inspector_window.cpp`)

Add a **Trace** tab (or extend the Debugger tab):
- Trap checkboxes: Undefined / Prefetch abort / Data abort.
- SWI trace: Enable, Halt-on-SWI, filter (name via `arm_disasm` SWI table, or hex range).
- Trace log list fed by `DrainTraceEvents`: `seq | PC | type | decoded`
  (SWI name from the existing 789-entry `arm_disasm.c` table, exception kind, or
  watch addr=value). Clear button, autoscroll toggle, dropped-count badge.
- "Log only (don't halt)" checkbox on the Add-watchpoint UI → sets `log_only`.

## Performance
- Logging-only paths cost nothing in the fast path: SWI logging lives in `opSWI`
  (always C, gated by `debugger_swi_trace_active`); exception logging lives in the
  cold `exception()`. Neither forces the per-instruction hooked path.
- Halt-trapping forces the checked path via `debugger_requires_instruction_hook`,
  same cost model as a set breakpoint.

## Phasing
- **A** — data model + ring + `Set`/`Drain` commands + Trace pane scaffold.
- **B** — SWI tracing (#2): highest value, exercises the whole pipeline.
- **C** — exception trapping (#1).
- **D** — logging watchpoints (#5): smallest, reuses everything.
