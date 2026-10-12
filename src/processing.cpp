#include "main.h"

const char* state_name(uint8_t state) {
  switch (state) {
    case STATE_SYSTEM_INIT:       return "STATE_SYSTEM_INIT";
    case STATE_EVALUATE_QUEUE:    return "STATE_EVALUATE_QUEUE";
    case STATE_NS_STRAIGHT:       return "STATE_NS_STRAIGHT";
    case STATE_N_PRIORITY:        return "STATE_N_PRIORITY";
    case STATE_S_PRIORITY:        return "STATE_S_PRIORITY";
    case STATE_WE_STRAIGHT:       return "STATE_WE_STRAIGHT";
    case STATE_W_PRIORITY:        return "STATE_W_PRIORITY";
    case STATE_E_PRIORITY:        return "STATE_E_PRIORITY";
    case STATE_YELLOW_TRANSITION: return "STATE_YELLOW_TRANSITION";
    case STATE_ALL_RED_CLEARANCE: return "STATE_ALL_RED_CLEARANCE";
    case STATE_FLASHING_YELLOW:   return "STATE_FLASHING_YELLOW";
    case STATE_EMERGENCY:         return "STATE_EMERGENCY";
    default:                      return "UNKNOWN";
  }
}

void print_debug(uint8_t state, uint8_t red, uint8_t yellow, uint8_t green) {
  Serial.printf("[DEBUG] State: %s (0x%02X)\n", state_name(state), state);
  Serial.printf("[DEBUG] Mask  -> R:0x%02X Y:0x%02X G:0x%02X\n", red, yellow, green);
  Serial.printf("[DEBUG] N-S:%s N-R:%s S-S:%s S-R:%s E-S:%s E-R:%s W-S:%s W-R:%s\n",
    (red & (1 << LANE_N_S)) ? "OFF" : "ON",
    (red & (1 << LANE_N_R)) ? "OFF" : "ON",
    (red & (1 << LANE_S_S)) ? "OFF" : "ON",
    (red & (1 << LANE_S_R)) ? "OFF" : "ON",
    (red & (1 << LANE_E_S)) ? "OFF" : "ON",
    (red & (1 << LANE_E_R)) ? "OFF" : "ON",
    (red & (1 << LANE_W_S)) ? "OFF" : "ON",
    (red & (1 << LANE_W_R)) ? "OFF" : "ON"
  );
  Serial.printf("[DEBUG]       N-S:%s N-R:%s S-S:%s S-R:%s E-S:%s E-R:%s W-S:%s W-R:%s\n",
    (yellow & (1 << LANE_N_S)) ? "OFF" : "ON",
    (yellow & (1 << LANE_N_R)) ? "OFF" : "ON",
    (yellow & (1 << LANE_S_S)) ? "OFF" : "ON",
    (yellow & (1 << LANE_S_R)) ? "OFF" : "ON",
    (yellow & (1 << LANE_E_S)) ? "OFF" : "ON",
    (yellow & (1 << LANE_E_R)) ? "OFF" : "ON",
    (yellow & (1 << LANE_W_S)) ? "OFF" : "ON",
    (yellow & (1 << LANE_W_R)) ? "OFF" : "ON"
  );
  Serial.printf("[DEBUG]       N-S:%s N-R:%s S-S:%s S-R:%s E-S:%s E-R:%s W-S:%s W-R:%s\n",
    (green & (1 << LANE_N_S)) ? "OFF" : "ON",
    (green & (1 << LANE_N_R)) ? "OFF" : "ON",
    (green & (1 << LANE_S_S)) ? "OFF" : "ON",
    (green & (1 << LANE_S_R)) ? "OFF" : "ON",
    (green & (1 << LANE_E_S)) ? "OFF" : "ON",
    (green & (1 << LANE_E_R)) ? "OFF" : "ON",
    (green & (1 << LANE_W_S)) ? "OFF" : "ON",
    (green & (1 << LANE_W_R)) ? "OFF" : "ON"
  );
  Serial.println();
}

void apply_traffic_state(uint8_t state, uint8_t &red, uint8_t &yellow, uint8_t &green) {
  switch (state) {
    case STATE_SYSTEM_INIT:
      red = 0x00;
      break;

    case STATE_EVALUATE_QUEUE:
      red = 0x00;
      break;

    case STATE_NS_STRAIGHT:
      set_light(red, LANE_N_S, false);
      set_light(red, LANE_N_R, false);
      set_light(red, LANE_S_S, false);
      set_light(red, LANE_S_R, false);
      set_light(red, LANE_E_S, true);
      set_light(red, LANE_E_R, false);
      set_light(red, LANE_W_S, true);
      set_light(red, LANE_W_R, false);
      set_light(green, LANE_N_S, true);
      set_light(green, LANE_S_S, true);
      break;

    case STATE_N_PRIORITY:
      set_light(red, LANE_N_S, false);
      set_light(red, LANE_N_R, false);
      set_light(red, LANE_S_S, true);
      set_light(red, LANE_S_R, true);
      set_light(red, LANE_E_S, true);
      set_light(red, LANE_E_R, true);
      set_light(red, LANE_W_S, true);
      set_light(red, LANE_W_R, true);
      set_light(green, LANE_N_S, true);
      set_light(green, LANE_N_R, true);
      break;

    case STATE_S_PRIORITY:
      set_light(red, LANE_N_S, true);
      set_light(red, LANE_N_R, true);
      set_light(red, LANE_S_S, false);
      set_light(red, LANE_S_R, false);
      set_light(red, LANE_E_S, true);
      set_light(red, LANE_E_R, true);
      set_light(red, LANE_W_S, true);
      set_light(red, LANE_W_R, true);
      set_light(green, LANE_S_S, true);
      set_light(green, LANE_S_R, true);
      break;

    case STATE_WE_STRAIGHT:
      set_light(red, LANE_N_S, true);
      set_light(red, LANE_N_R, true);
      set_light(red, LANE_S_S, true);
      set_light(red, LANE_S_R, true);
      set_light(red, LANE_E_S, false);
      set_light(red, LANE_E_R, true);
      set_light(red, LANE_W_S, false);
      set_light(red, LANE_W_R, true);
      set_light(green, LANE_E_S, true);
      set_light(green, LANE_W_S, true);
      break;

    case STATE_W_PRIORITY:
      set_light(red, LANE_N_S, true);
      set_light(red, LANE_N_R, true);
      set_light(red, LANE_S_S, true);
      set_light(red, LANE_S_R, true);
      set_light(red, LANE_E_S, true);
      set_light(red, LANE_E_R, true);
      set_light(red, LANE_W_S, false);
      set_light(red, LANE_W_R, false);
      set_light(green, LANE_W_S, true);
      set_light(green, LANE_W_R, true);
      break;

    case STATE_E_PRIORITY:
      set_light(red, LANE_N_S, true);
      set_light(red, LANE_N_R, true);
      set_light(red, LANE_S_S, true);
      set_light(red, LANE_S_R, true);
      set_light(red, LANE_E_S, false);
      set_light(red, LANE_E_R, false);
      set_light(red, LANE_W_S, true);
      set_light(red, LANE_W_R, true);
      set_light(green, LANE_E_S, true);
      set_light(green, LANE_E_R, true);
      break;

    case STATE_YELLOW_TRANSITION:
      red = 0xFF;
      yellow = 0x00;
      green = 0xFF;
      break;

    case STATE_ALL_RED_CLEARANCE:
      red = 0x00;
      break;

    case STATE_FLASHING_YELLOW:
      red = 0xFF;
      yellow = 0x00;
      green = 0xFF;
      break;

    case STATE_EMERGENCY:
      red = 0x00;
      break;

    default:
      break;
  }
}

void processing_command(uint8_t *data, int len) {
  if (len < 1) return;

  uint8_t cmd = data[0];

  switch (cmd) {
    case CMD_WRITE: {
      if (len < 2) return;
      uint8_t state = data[1];
      Serial.printf("[SLAVE] CMD_WRITE -> %s (0x%02X)\n", state_name(state), state);
      apply_state_to_registers(state);
      break;
    }

    case CMD_UTILS: {
      if (len < 2) return;
      uint8_t util = data[1];
      switch (util) {
        case UTILS_HELP:
          Serial.println("[SLAVE] UTILS_HELP:");
          Serial.println("[SLAVE]   CMD_WRITE     = 0x01 <STATE>");
          Serial.println("[SLAVE]   CMD_UTILS     = 0x02 <UTILS_*>");
          Serial.println("[SLAVE]   CMD_LOAD_CYCLE= 0x03 <COUNT> <PHASE> <DUR> ...");
          Serial.println("[SLAVE]   CMD_FORCE_NEXT= 0x04");
          Serial.println("[SLAVE]   CMD_EMERGENCY = 0x05");
          Serial.println("[SLAVE]   CMD_RESUME    = 0x06");
          Serial.println("[SLAVE]   CMD_CYCLE_DONE= 0x07 (TX)");
          Serial.println("[SLAVE]   CMD_REQ_CYCLE = 0x08 (TX)");
          Serial.println("[SLAVE]     UTILS_HELP = 0x01");
          Serial.println("[SLAVE]     UTILS_SIMULATE_ON = 0x02");
          Serial.println("[SLAVE]     UTILS_SIMULATE_OFF = 0x03");
          break;
        case UTILS_SIMULATE_ON:
          simulate_active = true;
          sim_index = 0;
          sim_last = millis();
          Serial.println("[SLAVE] Simulasi diaktifkan");
          break;
        case UTILS_SIMULATE_OFF:
          simulate_active = false;
          Serial.println("[SLAVE] Simulasi dihentikan");
          break;
        default:
          Serial.printf("[SLAVE] Unknown UTILS: 0x%02X\n", util);
          break;
      }
      break;
    }

    case CMD_LOAD_CYCLE: {
      if (len < 2) return;
      uint8_t count = data[1];
      if (count == 0 || count > MAX_CYCLE_STEPS) {
        Serial.println("[SLAVE] CMD_LOAD_CYCLE: invalid phase count");
        break;
      }
      uint8_t expected = 2 + count * 3; // CMD(1) + COUNT(1) + fase*3
      if (len < expected) {
        Serial.println("[SLAVE] CMD_LOAD_CYCLE: payload too short");
        break;
      }

      // Simpan ke nextCycle (shadow buffer) - overwrite jika sudah ada
      // (jika Raspi kirim >1 plan, plan terbaru yang dipakai)
      // Format payload: [COUNT] [PHASE_1] [DUR_1_hi] [DUR_1_lo] ...
      for (uint8_t i = 0; i < count; i++) {
        nextCycle[i].phase_id = data[2 + i * 3];
        nextCycle[i].duration_sec = ((uint16_t)data[2 + i * 3 + 1] << 8) | data[2 + i * 3 + 2];
      }
      nextCycleCount = count;
      hasNewCycle = true;
      Serial.printf("[SLAVE] CMD_LOAD_CYCLE: %d phases staged (nextCycle)\n", count);

      // Jika belum ada cycle berjalan (boot pertama) atau sedang failsafe:
      // langsung aktifkan tanpa menunggu akhir siklus
      if (fsmState == FSM_IDLE ||
          fsmState == FSM_FAILSAFE_RED ||
          fsmState == FSM_FAILSAFE_FLASHING) {
        activate_next_cycle();
        Serial.println("[SLAVE] Cycle activated immediately");
      }
      break;
    }

    case CMD_FORCE_NEXT: {
      if (fsmState == FSM_GREEN) {
        apply_state_to_registers(STATE_YELLOW_TRANSITION);
        fsmState = FSM_YELLOW;
        fsmTimerStart = millis();
        Serial.println("[SLAVE] CMD_FORCE_NEXT: GREEN -> YELLOW");
      } else if (fsmState == FSM_YELLOW) {
        apply_state_to_registers(STATE_ALL_RED_CLEARANCE);
        fsmState = FSM_RED;
        fsmTimerStart = millis();
        Serial.println("[SLAVE] CMD_FORCE_NEXT: YELLOW -> RED");
      } else if (fsmState == FSM_RED) {
        start_next_phase_or_end();
        Serial.println("[SLAVE] CMD_FORCE_NEXT: RED -> NEXT PHASE");
      } else {
        Serial.println("[SLAVE] CMD_FORCE_NEXT: ignored (not in cycle)");
      }
      break;
    }

    case CMD_EMERGENCY: {
      if (fsmState != FSM_EMERGENCY) {
        if (currentCycleCount > 0) {
          savedElapsedMs = millis() - cycleElapsedStart;
        }
        fsmState = FSM_EMERGENCY;
        apply_state_to_registers(STATE_EMERGENCY);
        Serial.println("[SLAVE] CMD_EMERGENCY: all red, timer frozen");
      }
      break;
    }

    case CMD_RESUME: {
      if (fsmState == FSM_EMERGENCY) {
        if (currentCycleCount == 0) {
          fsmState = FSM_IDLE;
          Serial.println("[SLAVE] CMD_RESUME: no cycle loaded, idle");
        } else {
          cycleElapsedStart = millis() - savedElapsedMs;
          start_green_phase();
          Serial.println("[SLAVE] CMD_RESUME: cycle resumed");
        }
      } else {
        Serial.println("[SLAVE] CMD_RESUME: not in emergency");
      }
      break;
    }

    default:
      Serial.printf("[SLAVE] Unknown COMMAND: 0x%02X\n", cmd);
      break;
  }
}
