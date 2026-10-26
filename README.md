# Smart Traffic Light - Slave Executor (ESP32)

Sistem slave executor untuk Smart Traffic Light 8 lajur. Raspberry Pi mengirim **Timing Plan** (satu siklus penuh) via Serial, lalu ESP32 mengeksekusinya secara mandiri (non-blocking) dengan mekanisme **Shadow Buffer**, **Prefetching 10 detik**, dan **Failsafe Degradation**.

## Arsitektur (PULL-BASED TIMING PLAN)

1. Raspberry Pi mengirim `CMD_LOAD_CYCLE` berisi array fase + durasi.
2. ESP32 menyimpan plan ke **`nextCycle`** (shadow buffer) tanpa mengganggu `currentCycle` yang sedang berjalan.
3. **10 detik sebelum siklus selesai**, ESP32 mengirim `CMD_REQ_CYCLE` (0x08) ke Raspberry Pi untuk meminta plan berikutnya (prefetch, 1x per siklus).
4. Saat siklus selesai:
   - Jika `hasNewCycle == true` → swap `nextCycle` → `currentCycle`, jalankan dari indeks 0.
   - Jika `hasNewCycle == false` (Raspi mati / kabel putus) → masuk **FAILSAFE**.
5. Jika `CMD_LOAD_CYCLE` masuk saat failsafe → langsung keluar failsafe dan jalankan plan tersebut dari indeks 0.

## Hardware

| Komponen | Keterangan |
|----------|------------|
| Board | ESP32-S3 |
| Shift Register | 3x 74HC595 (daisy-chain) |
| Relay | Active LOW |
| Jumlah Lampu | 24 (8 Merah, 8 Kuning, 8 Hijau) |
| Jumlah Lajur | 8 (4 pendekatan × straight + right turn) |

### Pin Assignment

| Pin ESP32 | Fungsi |
|-----------|--------|
| GPIO 2 | DATA_PIN (74HC595) |
| GPIO 4 | CLOCK_PIN (74HC595) |
| GPIO 16 | LATCH_PIN (74HC595) |

### Lane Mapping (8 lanes, numbered by ID)

| Lane ID | Bit | Physical (LHT Indonesia, clockwise N,E,S,W) |
|---------|-----|---------------------------------------------|
| 1 | 0 | North Straight |
| 2 | 1 | North Right Turn |
| 3 | 2 | East Straight |
| 4 | 3 | East Right Turn |
| 5 | 4 | South Straight |
| 6 | 5 | South Right Turn |
| 7 | 6 | West Straight |
| 8 | 7 | West Right Turn |

> Lane IDs 1-8 are used directly in code. Bit position = lane_id - 1.

## Protocol Serial

Format paket (RX maupun TX):

```
[0xAB] [0xCD] [0xFF] [LEN] [CMD] [DATA...]
```

| Byte | Keterangan |
|------|------------|
| 0 | Header 1: `0xAB` |
| 1 | Header 2: `0xCD` |
| 2 | Header 3: `0xFF` |
| 3 | Length: jumlah byte payload (CMD + DATA) |
| 4..N | Payload |

## Command Table

### RX Command (Raspberry Pi -> ESP32)

| Hex | Nama | Deskripsi |
|-----|------|-----------|
| `0x01` | `CMD_WRITE` | Tulis state manual ke lampu (override sesaat) |
| `0x03` | `CMD_LOAD_CYCLE` | Muat Timing Plan ke shadow buffer `nextCycle` |
| `0x04` | `CMD_FORCE_NEXT` | Lewati fase Hijau saat ini (Phase Skip) |
| `0x05` | `CMD_EMERGENCY` | Darurat: semua merah, timer dibekukan |
| `0x06` | `CMD_RESUME` | Lanjutkan siklus setelah darurat |

### TX Report (ESP32 -> Raspberry Pi)

| Hex | Nama | Paket | Deskripsi |
|-----|------|-------|-----------|
| `0x07` | `CMD_CYCLE_DONE` | `AB CD FF 01 07` | Laporan: siklus penuh selesai |
| `0x08` | `CMD_REQ_CYCLE` | `AB CD FF 01 08` | Permintaan: kirim Timing Plan terbaru (payload kosong) |

### Traffic State (`phase_id` dalam plan, atau payload `CMD_WRITE`)

**System / Transition States**

| Hex | Nama | Deskripsi |
|-----|------|-----------|
| `0x00` | `STATE_SYSTEM_INIT` | Semua merah |
| `0x01` | `STATE_EVALUATE_QUEUE` | Semua merah |
| `0x20` | `STATE_YELLOW_TRANSITION` | Semua kuning |
| `0x21` | `STATE_ALL_RED_CLEARANCE` | Semua merah |
| `0x22` | `STATE_FLASHING_YELLOW` | Semua kuning (failsafe berkedip) |
| `0x23` | `STATE_EMERGENCY` | Semua merah |

**Individual Lane Green States (8-junction mode)** — each lane gets its own green phase

| Hex | Nama | Green | Red |
|-----|------|-------|-----|
| `0x10` | `STATE_LANE_1_GREEN` | Lane 1 | 2-8 |
| `0x11` | `STATE_LANE_2_GREEN` | Lane 2 | 1,3-8 |
| `0x12` | `STATE_LANE_3_GREEN` | Lane 3 | 1-2,4-8 |
| `0x13` | `STATE_LANE_4_GREEN` | Lane 4 | 1-3,5-8 |
| `0x14` | `STATE_LANE_5_GREEN` | Lane 5 | 1-4,6-8 |
| `0x15` | `STATE_LANE_6_GREEN` | Lane 6 | 1-5,7-8 |
| `0x16` | `STATE_LANE_7_GREEN` | Lane 7 | 1-6,8 |
| `0x17` | `STATE_LANE_8_GREEN` | Lane 8 | 1-7 |

**Approach Pair Green States (4-junction mode)** — straight + right turn for one approach

| Hex | Nama | Green | Red |
|-----|------|-------|-----|
| `0x18` | `STATE_APPROACH_1_2_GREEN` | Lanes 1,2 | 3-8 |
| `0x19` | `STATE_APPROACH_3_4_GREEN` | Lanes 3,4 | 1,2,5-8 |
| `0x1A` | `STATE_APPROACH_5_6_GREEN` | Lanes 5,6 | 1-4,7-8 |
| `0x1B` | `STATE_APPROACH_7_8_GREEN` | Lanes 7,8 | 1-6 |

**Opposing Pair Green States (LHT right-turn-on-red)**

| Hex | Nama | Green | Red (ON) | Red (OFF = right-on-red) |
|-----|------|-------|----------|--------------------------|
| `0x1C` | `STATE_OPPOSING_1_3_GREEN` | Lanes 1,3 | 5,7 | 2,4,6,8 |
| `0x1D` | `STATE_OPPOSING_5_7_GREEN` | Lanes 5,7 | 1,3 | 2,4,6,8 |


### Transition

| State | Merah | Kuning | Hijau |
|-------|-------|--------|-------|
| `STATE_YELLOW_TRANSITION` | OFF | ON | OFF |
| `STATE_ALL_RED_CLEARANCE` | ON | OFF | OFF |
| `STATE_FLASHING_YELLOW` | OFF | ON (blink) | OFF |
| `STATE_EMERGENCY` | ON | OFF | OFF |

## Timing Plan Format (`CMD_LOAD_CYCLE`, 0x03)

Payload: `[JUMLAH_FASE] [FASE_1_ID] [DURASI_1] [FASE_2_ID] [DURASI_2] ...`

- `JUMLAH_FASE`: 1 byte (maksimal 8 fase)
- `FASE_x_ID`: 1 byte (TrafficState hijau, mis. `0x1C`)
- `DURASI_x`: 2 byte big-endian (`uint16_t`, satuan detik)

### Contoh: 4-junction cycle (opposing pairs, LHT right-on-red)

```
AB CD FF 0B 03 03 1C 00 1E 1D 00 1E 1C 00 1E
```

- Header `AB CD FF` + LEN=11 + CMD=03
- COUNT=03
- Fase 1: `0x1C` (OPPOSING_1_3) = 30 detik
- Fase 2: `0x1D` (OPPOSING_5_7) = 30 detik
- Fase 3: `0x1C` (OPPOSING_1_3) = 30 detik

### Contoh: 8-junction cycle (individual lanes)

```
AB CD FF 10 03 08 10 00 1E 12 00 1E 14 00 1E
```

- COUNT=08
- Fase 1: `0x10` (LANE_1) = 30 detik
- Fase 2: `0x12` (LANE_3) = 30 detik
- Fase 3: `0x14` (LANE_5) = 30 detik
- ... (4 more phases)

> Catatan: jika Raspberry Pi mengirim lebih dari satu plan sebelum siklus selesai, `nextCycle` lama **dibuang** dan digantikan plan terbaru.

## FSM Eksekusi (non-blocking, `millis()`)

Setiap fase dalam `currentCycle` dieksekusi berurutan:

```
FSM_GREEN (durasi sesuai plan)
   -> FSM_YELLOW (3 detik, statis)
   -> FSM_RED (2 detik, All-Red Clearance)
   -> fase berikutnya
   -> ... habis -> CMD_CYCLE_DONE -> swap / failsafe
```

## Prefetching

- Total durasi `currentCycle` dihitung: `SUM(durasi_hijau + 3s kuning + 2s merah)`.
- Tepat **10 detik sebelum siklus berakhir**, ESP32 mengirim `CMD_REQ_CYCLE` (`AB CD FF 01 08`) ke Raspberry Pi.
- Request hanya dikirim **1 kali per siklus** (`reqCycleSent`).

## Failsafe (Graceful Degradation)

Dipicu saat siklus selesai tetapi `hasNewCycle == false` (Raspi mati / kabel putus):

| Tahap | State | Perilaku |
|-------|-------|----------|
| 1 | `FSM_FAILSAFE_RED` | Semua Merah selama **5 detik** + kirim `CMD_REQ_CYCLE` sekali |
| 2 | `FSM_FAILSAFE_FLASHING` | Kuning berkedip tiap **500ms** + kirim `CMD_REQ_CYCLE` setiap **2 detik** |

Jika `CMD_LOAD_CYCLE` masuk kapan pun selama failsafe → langsung keluar failsafe dan menjalankan plan tersebut dari indeks 0.

## Contoh Pengiriman

### Muat Timing Plan (opposing pairs, 2 fase)
```
AB CD FF 0B 03 03 1C 00 1E 1D 00 1E 1C 00 1E
```

### Force Next (skip fase)
```
AB CD FF 01 04
```

### Emergency
```
AB CD FF 01 05
```

### Resume
```
AB CD FF 01 06
```

### Manual Write (state sesaat)
```
AB CD FF 02 01 1C
```

## Struktur File

```
src/
  main.h          - Enum, define, struct CycleStep, deklarasi global, prototipe
  main.cpp        - Setup, loop, serial_recv (parser AB CD FF), shiftOut driver
  processing.cpp  - processing_command, apply_traffic_state, handler CMD_LOAD_CYCLE dll.
  timer_fsm.cpp   - Shadow buffer, run_timer_fsm(), prefetch, failsafe, TX report
  callback.cpp    - on_serial_recv_cb
```

## Build & Upload

```bash
pio run --target upload
pio device monitor
```

## Catatan

- Relay menggunakan logika **Active LOW** (bit 0 = ON, bit 1 = OFF).
- 3x 74HC595 di-daisy-chain: shift out urutan `merah -> kuning -> hijau` dengan `MSBFIRST`.
- Semua operasi waktu menggunakan `millis()` (**non-blocking**, tanpa `delay()` di loop).
- Serial receiver non-blocking dengan header `AB CD FF` dan kalkulasi `LENGTH`.
- Lane IDs 1-8 digunakan langsung di kode; bit position = lane_id - 1.
- LHT (Indonesia): right turn tidak bentrok dengan lalu lintas berlawanan arah. Pada state `OPPOSING_*_GREEN`, lampu kanan semua mati (red OFF) untuk right-turn-on-red.

## Breaking Changes

- TrafficState enum values berubah sepenuhnya.
- Konstanta `LANE_N_S` dll. dihapus; gunakan literal ID 1-8.
- `set_light()` menerima lane_id (1-8) bukan bit position (0-7).
- Raspberry Pi master harus diupdate untuk menggunakan nilai state baru.
