# Smart Traffic Light - Slave Executor (ESP32)

Sistem slave executor untuk Smart Traffic Light 8 lajur. Raspberry Pi mengirim **Timing Plan** (satu siklus penuh) via Serial, lalu ESP32 mengeksekusinya secara mandiri (non-blocking) dengan mekanisme **Shadow Buffer**, **Prefetching 10 detik**, dan **Failsafe Degradation**.

## Arsitektur (PULL-BASED TIMING PLAN)

1. Raspberry Pi mengirim `CMD_LOAD_CYCLE` berisi array fase + durasi.
2. ESP32 menyimpan plan ke **`nextCycle`** (shadow buffer) tanpa mengganggu `currentCycle` yang sedang berjalan.
3. **10 detik sebelum siklus selesai**, ESP32 mengirim `CMD_REQ_CYCLE` (0x08) ke Raspberry Pi untuk meminta plan berikutnya (prefetch, 1x per siklus).
4. Saat siklus selesai:
   - Jika `hasNewCycle == true` -> swap `nextCycle` -> `currentCycle`, jalankan dari indeks 0.
   - Jika `hasNewCycle == false` (Raspi mati / kabel putus) -> masuk **FAILSAFE**.
5. Jika `CMD_LOAD_CYCLE` masuk saat failsafe -> langsung keluar failsafe dan jalankan plan tersebut.

## Hardware

| Komponen | Keterangan |
|----------|------------|
| Board | ESP32-S3 |
| Shift Register | 3x 74HC595 (daisy-chain) |
| Relay | Active LOW |
| Jumlah Lampu | 24 (8 Merah, 8 Kuning, 8 Hijau) |
| Jumlah Lajur | 8 (Utara Lurus/Kanan, Selatan Lurus/Kanan, Timur Lurus/Kanan, Barat Lurus/Kanan) |

### Pin Assignment

| Pin ESP32 | Fungsi |
|-----------|--------|
| GPIO 2 | DATA_PIN (74HC595) |
| GPIO 4 | CLOCK_PIN (74HC595) |
| GPIO 16 | LATCH_PIN (74HC595) |

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
| `0x02` | `CMD_UTILS` | Perintah utilitas (help, simulasi legacy) |
| `0x03` | `CMD_LOAD_CYCLE` | Muat Timing Plan ke shadow buffer `nextCycle` |
| `0x04` | `CMD_FORCE_NEXT` | Lewati fase Hijau saat ini (Phase Skip) |
| `0x05` | `CMD_EMERGENCY` | Darurat: semua merah, timer dibekukan |
| `0x06` | `CMD_RESUME` | Lanjutkan siklus setelah darurat |

### TX Report (ESP32 -> Raspberry Pi)

| Hex | Nama | Paket | Deskripsi |
|-----|------|-------|-----------|
| `0x07` | `CMD_CYCLE_DONE` | `AB CD FF 01 07` | Laporan: siklus penuh selesai |
| `0x08` | `CMD_REQ_CYCLE` | `AB CD FF 01 08` | Permintaan: kirim Timing Plan terbaru (payload kosong) |

### Utils Command (`CMD_UTILS`)

| Hex | Nama | Deskripsi |
|-----|------|-----------|
| `0x01` | `UTILS_HELP` | Tampilkan daftar perintah |
| `0x02` | `UTILS_SIMULATE_ON` | Aktifkan simulasi legacy |
| `0x03` | `UTILS_SIMULATE_OFF` | Hentikan simulasi legacy |

### Traffic State (`phase_id` dalam plan, atau payload `CMD_WRITE`)

| Hex | Nama | Deskripsi |
|-----|------|-----------|
| `0x00` | `STATE_SYSTEM_INIT` | Semua merah |
| `0x01` | `STATE_EVALUATE_QUEUE` | Semua merah |
| `0x0A` | `STATE_NS_STRAIGHT` | Utara & Selatan Lurus Hijau, lainnya Merah |
| `0x0B` | `STATE_N_PRIORITY` | Utara Lurus & Kanan Hijau, lainnya Merah |
| `0x0C` | `STATE_S_PRIORITY` | Selatan Lurus & Kanan Hijau, lainnya Merah |
| `0x0D` | `STATE_WE_STRAIGHT` | Timur & Barat Lurus Hijau, lainnya Merah |
| `0x0E` | `STATE_W_PRIORITY` | Barat Lurus & Kanan Hijau, lainnya Merah |
| `0x0F` | `STATE_E_PRIORITY` | Timur Lurus & Kanan Hijau, lainnya Merah |
| `0x14` | `STATE_YELLOW_TRANSITION` | Semua lampu Kuning |
| `0x15` | `STATE_ALL_RED_CLEARANCE` | Semua lampu Merah |
| `0x16` | `STATE_FLASHING_YELLOW` | Semua Kuning (failsafe berkedip) |
| `0x17` | `STATE_EMERGENCY` | Semua lampu Merah |

## Timing Plan Format (`CMD_LOAD_CYCLE`, 0x03)

Payload: `[JUMLAH_FASE] [FASE_1_ID] [DURASI_1] [FASE_2_ID] [DURASI_2] ...`

- `JUMLAH_FASE`: 1 byte (maksimal 8 fase)
- `FASE_x_ID`: 1 byte (TrafficState hijau, mis. `0x0A`)
- `DURASI_x`: 2 byte big-endian (`uint16_t`, satuan detik)

Contoh plan 3 fase:
- `03` = 3 fase
- `0A 00 1E` = Fase 1: `STATE_NS_STRAIGHT`, 30 detik
- `0D 00 1E` = Fase 2: `STATE_WE_STRAIGHT`, 30 detik
- `0B 00 0A` = Fase 3: `STATE_N_PRIORITY`, 10 detik

Paket lengkap:
```
AB CD FF 0B 03 03 0A 00 1E 0D 00 1E 0B 00 0A
```
(Header `AB CD FF` + LEN=11/`0x0B` + CMD=`03` + payload: COUNT=`03` lalu 3×(PHASE+DUR))

> Catatan: jika Raspi mengirim lebih dari satu plan sebelum siklus selesai, `nextCycle` lama **dibuang** dan digantikan plan terbaru.

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

Jika `CMD_LOAD_CYCLE` masuk kapan pun selama failsafe -> langsung keluar failsafe dan menjalankan plan tersebut dari indeks 0.

## Contoh Pengiriman

### Muat Timing Plan (3 fase)
```
AB CD FF 0B 03 03 0A 00 1E 0D 00 1E 0B 00 0A
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
AB CD FF 02 01 0A
```

## Struktur File

```
src/
  main.h          - Enum, define, struct CycleStep, deklarasi global, prototipe
  main.cpp        - Setup, loop, serial_recv (parser AB CD FF), shiftOut driver
  processing.cpp  - processing_command, apply_traffic_state, handler CMD_LOAD_CYCLE dll.
  timer_fsm.cpp   - Shadow buffer, run_timer_fsm(), prefetch, failsafe, TX report
  simulate.cpp    - Simulasi legacy (opsional)
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
