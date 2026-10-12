#include "main.h"

// ---- Shadow buffers & FSM state ----
struct CycleStep currentCycle[MAX_CYCLE_STEPS];
struct CycleStep nextCycle[MAX_CYCLE_STEPS];
uint8_t currentCycleCount = 0;
uint8_t nextCycleCount = 0;
bool hasNewCycle = false;
uint8_t cycleIndex = 0;
enum FsmState fsmState = FSM_IDLE;
unsigned long fsmTimerStart = 0;
unsigned long cycleElapsedStart = 0;
uint32_t cycleTotalDurationMs = 0;
bool reqCycleSent = false;
bool reqSentInFailsafeRed = false;
unsigned long failsafeLastReq = 0;
bool flashingState = false;
uint32_t savedElapsedMs = 0;

uint32_t compute_cycle_duration_ms(struct CycleStep *steps, uint8_t count) {
  uint32_t total = 0;
  for (uint8_t i = 0; i < count; i++) {
    total += (uint32_t)steps[i].duration_sec * 1000UL; // hijau
    total += FSM_YELLOW_DURATION_MS;                    // kuning
    total += FSM_RED_DURATION_MS;                       // all-red clearance
  }
  return total;
}

void apply_state_to_registers(uint8_t state, bool debug) {
  uint8_t red = 0xFF, yellow = 0xFF, green = 0xFF;
  apply_traffic_state(state, red, yellow, green);
  if (debug) print_debug(state, red, yellow, green);
  update_shift_registers(red, yellow, green);
}

// TX (ESP32 -> Raspberry Pi): minta timing plan terbaru
// Paket: AB CD FF 01 08  (LEN=1, hanya CMD, tanpa data)
void send_req_cycle() {
  Serial.write(0xAB);
  Serial.write(0xCD);
  Serial.write(0xFF);
  Serial.write(0x01);
  Serial.write(CMD_REQ_CYCLE);
}

// TX (ESP32 -> Raspberry Pi): laporan siklus selesai
// Paket: AB CD FF 01 07
void send_cycle_done() {
  Serial.write(0xAB);
  Serial.write(0xCD);
  Serial.write(0xFF);
  Serial.write(0x01);
  Serial.write(CMD_CYCLE_DONE);
}

void start_green_phase() {
  apply_state_to_registers(currentCycle[cycleIndex].phase_id);
  fsmState = FSM_GREEN;
  fsmTimerStart = millis();
}

void activate_next_cycle() {
  memcpy(currentCycle, nextCycle, sizeof(currentCycle));
  currentCycleCount = nextCycleCount;
  hasNewCycle = false;
  cycleIndex = 0;
  cycleTotalDurationMs = compute_cycle_duration_ms(currentCycle, currentCycleCount);
  cycleElapsedStart = millis();
  reqCycleSent = false;
  start_green_phase();
}

void start_next_phase_or_end() {
  cycleIndex++;
  if (cycleIndex >= currentCycleCount) {
    // ---- Siklus selesai ----
    send_cycle_done();

    if (hasNewCycle) {
      // Swap shadow buffer: nextCycle -> currentCycle
      activate_next_cycle();
      Serial.println("[FSM] Cycle swapped from nextCycle");
    } else {
      // Raspi mati / kabel putus -> FAILSAFE Tahap 1
      fsmState = FSM_FAILSAFE_RED;
      fsmTimerStart = millis();
      reqSentInFailsafeRed = false;
      apply_state_to_registers(STATE_ALL_RED_CLEARANCE);
      Serial.println("[FSM] Cycle ended, no new plan -> FAILSAFE RED");
    }
  } else {
    start_green_phase();
  }
}

void run_timer_fsm() {
  unsigned long now = millis();

  // --------- minta plan baru 10 detik sebelum siklus berakhir ----
  // Dipastikan hanya 1x minta per siklus
  if ((fsmState == FSM_GREEN || fsmState == FSM_YELLOW || fsmState == FSM_RED) &&
      !reqCycleSent && currentCycleCount > 0) {
    uint32_t elapsed = now - cycleElapsedStart;
    uint32_t threshold = (cycleTotalDurationMs > PREFETCH_THRESHOLD_MS)
                           ? (cycleTotalDurationMs - PREFETCH_THRESHOLD_MS)
                           : 0UL;
    if (elapsed >= threshold) {
      send_req_cycle();
      reqCycleSent = true;
      Serial.println("[FSM] Prefetch: CMD_REQ_CYCLE sent (T-10s)");
    }
  }

  switch (fsmState) {
    case FSM_IDLE:
      // Menunggu CMD_LOAD_CYCLE pertama
      break;

    case FSM_GREEN:
      if (now - fsmTimerStart >= (unsigned long)currentCycle[cycleIndex].duration_sec * 1000UL) {
        apply_state_to_registers(STATE_YELLOW_TRANSITION);
        fsmState = FSM_YELLOW;
        fsmTimerStart = now;
      }
      break;

    case FSM_YELLOW:
      if (now - fsmTimerStart >= FSM_YELLOW_DURATION_MS) {
        apply_state_to_registers(STATE_ALL_RED_CLEARANCE);
        fsmState = FSM_RED;
        fsmTimerStart = now;
      }
      break;

    case FSM_RED:
      if (now - fsmTimerStart >= FSM_RED_DURATION_MS) {
        start_next_phase_or_end();
      }
      break;

    case FSM_FAILSAFE_RED:
      // Tahap 1: semua merah 5 detik + kirim CMD_REQ_CYCLE sekali lagi
      if (!reqSentInFailsafeRed) {
        send_req_cycle();
        reqSentInFailsafeRed = true;
        Serial.println("[FSM] Failsafe RED: CMD_REQ_CYCLE sent");
      }
      if (now - fsmTimerStart >= FAILSAFE_RED_DURATION_MS) {
        // awktu nunggu
        fsmState = FSM_FAILSAFE_FLASHING;
        fsmTimerStart = now;
        failsafeLastReq = now;
        flashingState = true;
        apply_state_to_registers(STATE_FLASHING_YELLOW, false);
        Serial.println("[FSM] Failsafe FLASHING started (retry every 2s)");
      }
      break;

    case FSM_FAILSAFE_FLASHING:
      // Tahap 2: kuning berkedip tiap 5s
      if (now - fsmTimerStart >= FAILSAFE_FLASH_INTERVAL_MS) {
        fsmTimerStart = now;
        flashingState = !flashingState;
        apply_state_to_registers(flashingState ? STATE_FLASHING_YELLOW
                                               : STATE_ALL_RED_CLEARANCE, false);
      }
      // Terus minta plan setiap 2 detik
      if (now - failsafeLastReq >= FAILSAFE_REQ_INTERVAL_MS) {
        failsafeLastReq = now;
        send_req_cycle();
      }
      break;

    case FSM_EMERGENCY:
      // Timer dibekukan, semua merah (diatur oleh CMD_EMERGENCY)
      break;

    default:
      break;
  }
}
