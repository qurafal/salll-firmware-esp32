# Smart Traffic Light - Slave Executor (ESP32)

Slave controller for an 8-stream traffic light system implementing orthogonal Micro-FSMs with a FreeRTOS-based coordinator.

## Architecture

- **Two FreeRTOS Tasks**: 
  - `Task_UART_Coordinator`: Handles UART frame parsing and command processing
  - `Task_FSM_Hardware`: Runs all Micro-FSM state machines, aggregates outputs, drives 74HC595 shift registers
- **8 Independent Micro-FSMs**: One per stream (0-7), 1:1 mapping to traffic lanes/streams
- **3x 74HC595 Shift Registers** (cascaded): 24-bit output for 8 streams × 3 lamps
  - Register 1 (bits 0-7): Green lamps
  - Register 2 (bits 8-15): Yellow lamps  
  - Register 3 (bits 16-23): Red lamps
- **System States**: `BOOT_CONFIG`, `RUNNING`, `FALLBACK_CYCLE`

## Micro-FSM States (Per Stream)

| State | Description |
|-------|-------------|
| `DISABLED_RED` | Red lamp on, no active request |
| `GREEN` | Green lamp active |
| `CLEARANCE_YELLOW` | Yellow clearance timer (local, non-blocking, uses `y_sec` from config) |
| `CLEARANCE_RED` | Red clearance timer (local, non-blocking, uses `r_sec` from config) |

**Transitions**: 
- Master sets Target=1 (GREEN): `DISABLED_RED` → `GREEN` (immediate if no conflict)
- Master sets Target=0 (RED): `GREEN` → `CLEARANCE_YELLOW` → `CLEARANCE_RED` → `DISABLED_RED` (auto-timed)

## Hardware

| Component | Details |
|-----------|---------|
| Board | ESP32-S3 |
| Shift Registers | 3x 74HC595 (daisy-chained) |
| Relays | Active LOW |
| Lamps | 24 (8 Red, 8 Yellow, 8 Green) |
| Streams/Lanes | 8 |

### Pin Assignment

| Pin | Function |
|-----|----------|
| GPIO 2 | DATA_PIN (74HC595) |
| GPIO 4 | CLOCK_PIN (74HC595) |
| GPIO 16 | LATCH_PIN (74HC595) |

### Stream Mapping

Streams 0-7 map directly to bit positions (bit = stream index). Physical mapping follows project convention.

## Binary Protocol

All frames follow strict format:

```
[0xAB] [0xCD] [0xFF] [LEN] [CMD] [DATA...]
```

| Byte | Description |
|------|-------------|
| 0 | Header 1: `0xAB` |
| 1 | Header 2: `0xCD` |
| 2 | Header 3: `0xFF` |
| 3 | Length: number of payload bytes (CMD + DATA). **LEN == 0 is rejected** |
| 4..N | Payload (CMD + DATA) |

## Commands

### Master → Slave (RX)

| Cmd | Name | Payload | Description |
|-----|------|---------|-------------|
| `0x10` | `CMD_INIT` | `[Y_SEC][R_SEC][Matrix[0..7]][FallbackData...]` | Initialize config. Enters `BOOT_CONFIG` on startup, must receive INIT to enter `RUNNING`. |
| `0x30` | `CMD_SET_STREAM` | `[StreamID][Target]` | Set stream state. Target=`1` → request GREEN, Target=`0` → request RED transition. Subject to conflict matrix interlock. |
| `0x40` | `CMD_HEARTBEAT` | None | Resets 5s watchdog (only in `RUNNING` mode). |
| `0x50` | `CMD_RESYNC` | None | Exit `FALLBACK_CYCLE` back to `BOOT_CONFIG` (clears shift registers). |

### Slave → Master (TX - ACKs)

All ACKs are 5-byte frames: `AB CD FF 01 <ACK_CMD>`

| Cmd | Name | Description |
|-----|------|-------------|
| `0x11` | `ACK_INIT_OK` | Config accepted, now in `RUNNING` mode |
| `0x31` | `ACK_TRANSITION` | Command accepted and transition executed |
| `0xFF` | `ACK_REJECTED` | Command rejected (conflict detected or invalid) |

## Conflict Matrix & Interlock

The Master sends an 8-byte conflict matrix in `CMD_INIT`:
- `Matrix[i]` = row for Stream `i`
- Bit `j` set in `Matrix[i]` means Stream `j` conflicts with Stream `i`

**Interlock Logic (on `CMD_SET_STREAM(i, 1)` for GREEN):**
1. Coordinator computes `ActiveMask` = bits set for streams in `GREEN`, `CLEARANCE_YELLOW`, or `CLEARANCE_RED`
2. Check: `conflict = ActiveMask & Matrix[i]`
3. If `conflict == 0`: Accept, transition stream `i` to `GREEN`, send `ACK_TRANSITION (0x31)`
4. If `conflict != 0`: Reject, send `ACK_REJECTED (0xFF)`

## Watchdog & Fallback

- **Watchdog**: 5 second timeout in `RUNNING` mode. Reset by `CMD_HEARTBEAT (0x40)`.
- **On Watchdog Timeout**: Enter `FALLBACK_CYCLE`. All streams forced to `DISABLED_RED`. Coordinator ignores `CMD_SET_STREAM` and `CMD_HEARTBEAT`, only accepts `CMD_RESYNC (0x50)`.
- **Fallback Execution**: Staggered loop over pairs `[Mask, Seconds]` from `FallbackData`. For each pair: set streams in `Mask` to `GREEN` for `Seconds`, then force through `YELLOW`/`RED` clearance timers, move to next pair (loops continuously).
- **Resync**: `CMD_RESYNC` in `FALLBACK_CYCLE` clears shift registers and returns to `BOOT_CONFIG`.

## Shift Register Output

Active-LOW relays: bit = 0 means lamp ON, bit = 1 means lamp OFF.

For each stream `i`:
- **GREEN**: Green ON (bit clear), Yellow OFF, Red OFF
- **CLEARANCE_YELLOW**: Green OFF, Yellow ON, Red OFF  
- **CLEARANCE_RED** / **DISABLED_RED**: Green OFF, Yellow OFF, Red ON

## File Structure

```
src/
  main.h          - Enums, structs, globals, task declarations
  main.cpp        - Setup, task creation, shift register driver
  comm.h/cpp      - ESP-NOW, serial frame parsing
  processing.cpp  - Command handlers for new protocol
  timer_fsm.cpp   - Micro-FSM tasks, aggregation, watchdog, fallback
  callback.cpp    - Serial receive callback
```

## Build & Upload

```bash
pio run --target upload
pio device monitor
```

## Notes

- Uses FreeRTOS with 2 tasks (Coordinator priority 2, FSM Hardware priority 1)
- All timing is non-blocking using `millis()`
- Serial parsing enforces `AB CD FF` header and rejects `LEN == 0`
- Shift registers updated at ~10ms intervals by FSM task
- ESP-NOW initialized but primarily used for debug output
