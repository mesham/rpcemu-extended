# Serial and parallel ports

RPCEmu (Spork Edition) emulates the Risc PC's serial and parallel hardware and lets
you redirect each port to the host: a log file, a virtual printer, or — for the
serial port — a TCP "modem" that dials real telnet BBSes.

Configure both from the menus:

- **Settings → Serial…**
- **Settings → Parallel…**

Settings are applied immediately and saved into the machine's `.cfg` file.

---

## Serial port

The Risc PC has a **single** hardware serial port: a 16550A UART in the SMC Super I/O
chip, memory-mapped at `0x3F8` (register spacing of 4 bytes). Its interrupt is wired
to **IOMD IRQ register B, bit 2** and gated by the UART's MCR **OUT2** line — RISC OS's
`Serial` module (`Serial710` driver) claims that IRQ and drives transmission from the
THRE (transmit-holding-register-empty) interrupt. The emulator models all of this, so
the standard RISC OS serial stack and ordinary terminal software work unchanged.


### Modes

| Mode | What it does |
| --- | --- |
| **Disabled** | No device attached. Guest sees a UART with nothing connected. |
| **Log to file** | Every byte the guest transmits is appended to a file on the host. |
| **TCP modem** | A Hayes-AT modem front end over a real TCP socket, with a telnet client layer. Dial telnet BBSes and other TCP services. |
| **Physical device** | Reserved for host `/dev/tty*` passthrough — **not yet implemented**. |

### Log to file

Pick **Log to file** and a path (e.g. `~/Documents/serial.log`). The raw byte stream
the guest sends to the UART is appended to that file (binary, unbuffered-flush). If you
leave the path empty, a default is used under the machine's data directory
(`machines/<name>/serial_serial.log`).

This is the simplest way to capture whatever a program sends to the serial port.

**Sending serial output from RISC OS**

- Most serial-aware applications (terminals, comms software) open the port directly.
- From BASIC you can transmit bytes with `OS_SerialOp`:
  ```basic
  SYS "OS_SerialOp", 3, ASC"H"   : REM send one byte ('H')
  ```
- The legacy serial *output stream* still works too:
  ```basic
  *FX 3,1        : REM route OS_WriteC output to the serial port
  PRINT "Hello"
  *FX 3,0        : REM restore normal output
  ```

### TCP modem (telnet)

Choose **TCP modem** and the port behaves like a Hayes-compatible modem. Point your
RISC OS terminal software (ANSITerm, !Connector/Hearsay, etc.) at the serial port and
"dial" with an AT command — the emulator turns the dial string into a real TCP
connection.

**Dialling**

```
ATDT hostname:port
ATDT bbs.example.com:23
ATDT 192.168.0.50:6400
ATD  hostname            (defaults to port 23, telnet)
```

- The text after `ATD`/`ATDT`/`ATDP` is parsed as `host[:port]` (a space before the
  port also works). The default port is **23** (telnet).
- On success the modem replies `CONNECT` and enters data mode; on DNS or connection
  failure it replies `NO CARRIER`.

**Telnet and binary-clean transfers**

The TCP modem is a telnet client: it answers telnet option negotiation, escapes/
unescapes `IAC` (`0xFF`), and negotiates **binary** + **suppress-go-ahead** so the
link is fully 8-bit transparent. It performs no CR/LF translation. This means
**X/Y/ZMODEM** (and other binary) file transfers run cleanly over a connected BBS.

**Returning to command mode / hanging up**

- `+++` — the Hayes escape sequence. It is **guard-timed** (about one second of
  silence before and after), so a literal `+++` occurring inside a file transfer is
  treated as data, not an escape. After a successful escape the modem replies `OK`
  and is in command mode while still connected.
- `ATO` — return to data (online) mode.
- `ATH` — hang up (close the connection).
- Dropping **DTR** also hangs up (Hayes `&D2` behaviour), so a terminal that lowers
  DTR on exit disconnects cleanly. `DCD` reflects the live connection.

**Supported AT commands**

| Command | Effect |
| --- | --- |
| `AT` | Returns `OK` (used to probe the modem). |
| `ATDT`/`ATD`/`ATDP <host[:port]>` | Dial a TCP/telnet connection. |
| `ATH` / `ATH0` | Hang up. |
| `ATO` | Return online after `+++`. |
| `ATE0` / `ATE1` | Command-mode echo off / on (default on). |
| `ATV0` / `ATV1` | Numeric / verbose result codes (default verbose). |
| `ATQ0` / `ATQ1` | Result codes on / suppressed. |
| `ATI` | Identify (`RPCEmu TCP modem`). |
| `ATZ`, `AT&F`, other init strings | Accepted and answered with `OK`. |

Result codes: `OK`, `CONNECT`, `NO CARRIER`, `ERROR` (verbose) or `0`,`1`,`3`,`4`
(numeric).

**Limitations**

- **DNS resolution is synchronous.** Dialling a host that is slow to resolve can
  briefly pause the emulator until the lookup completes; dialling a literal IP address
  avoids this.
- The modem operates in **telnet mode**. Pure raw-TCP services that send literal
  `0xFF` data bytes are not the target use case (telnet would double them); BBSes and
  telnet services are.
- Physical `/dev/tty*` passthrough is not implemented.

---

## Parallel port

The Risc PC's Centronics parallel port is emulated through the Super I/O chip. As with
serial, you can redirect it to the host.

### Modes

| Mode | What it does |
| --- | --- |
| **Disabled** | No device attached. |
| **Log to file** | Raw bytes written to the parallel port are appended to a file. |
| **Virtual printer** | Captures each print job to a `.prn` file, with optional PDF conversion. |
| **Physical device** | Reserved for host passthrough — **not yet implemented**. |

### Log to file

Captures the raw parallel byte stream to the chosen file. Useful for inspecting exactly
what a printer driver emits.

### Virtual printer

The virtual printer presents a ready, online Centronics device to RISC OS and collects
each print job into a separate `.prn` file in the chosen output folder (default:
`machines/<name>/printjobs/`).

To print from RISC OS:

1. In **!Printers**, choose a printer driver whose output goes to the **parallel**
   (Centronics) port — typically a PostScript driver.
2. Print as normal. Each job is written to a new `.prn` file.

If the emulator was built with Ghostscript support (`libgs-dev`), tick **Also create
PDF files** to convert PostScript `.prn` jobs to PDF in-process — no external tools
needed. See [COMPILE.md](../COMPILE.md) for the GhostPDL build options.

---

## Configuration keys

These live under the `[General]` group of each machine's `.cfg` file and are written
by the Serial/Parallel dialogs; you don't normally edit them by hand.

| Key | Meaning |
| --- | --- |
| `serial_com1_mode` | `0` disabled, `1` log to file, `2` TCP modem, `3` device (unimplemented) |
| `serial_com1_log` | Log file path (log mode) |
| `serial_com1_device` | Host device path (reserved) |
| `parallel_mode` | `0` disabled, `1` log to file, `2` virtual printer, `3` device (unimplemented) |
| `parallel_log` | Log file path (log mode) |
| `parallel_device` | Host device path (reserved) |
| `printer_output_path` | Folder for captured `.prn` jobs (virtual printer) |
| `printer_auto_pdf` | `1` to also write a PDF per job (needs Ghostscript build) |

> `serial_com2_*` keys may exist in older configuration files. They are ignored — the
> Risc PC has only one serial port.

---

## Implementation notes

| File | Role |
| --- | --- |
| `src/superio.c` | 16550 UART(s) and Centronics registers; serial IRQ on IOMD IRQB bit 2, gated by MCR OUT2 |
| `src/serial.c` | Virtual serial "bus" connecting the UART to a backend device |
| `src/parallel.c` | Virtual parallel bus |
| `src/printer.c` | Virtual printer (job capture, optional PDF via `print_convert.c`) |
| `src/peripheral_config.c` | Backends: file logging, the telnet TCP modem, and printer wiring |
| `src/gui/serial_dialog.cpp`, `src/gui/parallel_dialog.cpp` | Configuration dialogs |

The TCP modem's socket I/O is non-blocking and serviced from `serial_modem_poll()` on
the emulation thread, so it never stalls emulation (aside from the synchronous DNS
lookup noted above). Incoming data is paced into the UART's 16-byte receive FIFO via
`superio_serial_rx_space()` so fast downloads don't overrun it.
