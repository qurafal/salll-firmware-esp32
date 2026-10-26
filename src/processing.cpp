#include "main.h"
#include "comm.h"

const char* state_name(uint8_t state) {
  switch (state) {
    case STATE_SYSTEM_INIT:       return "STATE_SYSTEM_INIT";
    case STATE_EVALUATE_QUEUE:    return "STATE_EVALUATE_QUEUE";

    case STATE_LANE_1_GREEN:      return "STATE_LANE_1_GREEN";
    case STATE_LANE_2_GREEN:      return "STATE_LANE_2_GREEN";
    case STATE_LANE_3_GREEN:      return "STATE_LANE_3_GREEN";
    case STATE_LANE_4_GREEN:      return "STATE_LANE_4_GREEN";
    case STATE_LANE_5_GREEN:      return "STATE_LANE_5_GREEN";
    case STATE_LANE_6_GREEN:      return "STATE_LANE_6_GREEN";
    case STATE_LANE_7_GREEN:      return "STATE_LANE_7_GREEN";
    case STATE_LANE_8_GREEN:      return "STATE_LANE_8_GREEN";

    case STATE_APPROACH_1_2_GREEN: return "STATE_APPROACH_1_2_GREEN";
    case STATE_APPROACH_3_4_GREEN: return "STATE_APPROACH_3_4_GREEN";
    case STATE_APPROACH_5_6_GREEN: return "STATE_APPROACH_5_6_GREEN";
    case STATE_APPROACH_7_8_GREEN: return "STATE_APPROACH_7_8_GREEN";
    case STATE_OPPOSING_1_5_GREEN:      return "STATE_OPPOSING_1_5_GREEN";
    case STATE_OPPOSING_3_7_GREEN:      return "STATE_OPPOSING_3_7_GREEN";

    case STATE_YELLOW_TRANSITION: return "STATE_YELLOW_TRANSITION";
    case STATE_ALL_RED_CLEARANCE: return "STATE_ALL_RED_CLEARANCE";
    case STATE_FLASHING_YELLOW:   return "STATE_FLASHING_YELLOW";
    case STATE_EMERGENCY:         return "STATE_EMERGENCY";

    default:                      return "UNKNOWN";
  }
}

void print_debug(uint8_t state, uint8_t red, uint8_t yellow, uint8_t green) {
  send_debug("[DEBUG] State: %s (0x%02X)\n", state_name(state), state);
  send_debug("[DEBUG] Mask  -> R:0x%02X Y:0x%02X G:0x%02X\n", red, yellow, green);
  send_debug("[DEBUG]       L1:%s L2:%s L3:%s L4:%s L5:%s L6:%s L7:%s L8:%s\n",
    (red & (1 << (1 - 1))) ? "OFF" : "ON",
    (red & (1 << (2 - 1))) ? "OFF" : "ON",
    (red & (1 << (3 - 1))) ? "OFF" : "ON",
    (red & (1 << (4 - 1))) ? "OFF" : "ON",
    (red & (1 << (5 - 1))) ? "OFF" : "ON",
    (red & (1 << (6 - 1))) ? "OFF" : "ON",
    (red & (1 << (7 - 1))) ? "OFF" : "ON",
    (red & (1 << (8 - 1))) ? "OFF" : "ON"
  );
  send_debug("[DEBUG]       L1:%s L2:%s L3:%s L4:%s L5:%s L6:%s L7:%s L8:%s\n",
    (yellow & (1 << (1 - 1))) ? "OFF" : "ON",
    (yellow & (1 << (2 - 1))) ? "OFF" : "ON",
    (yellow & (1 << (3 - 1))) ? "OFF" : "ON",
    (yellow & (1 << (4 - 1))) ? "OFF" : "ON",
    (yellow & (1 << (5 - 1))) ? "OFF" : "ON",
    (yellow & (1 << (6 - 1))) ? "OFF" : "ON",
    (yellow & (1 << (7 - 1))) ? "OFF" : "ON",
    (yellow & (1 << (8 - 1))) ? "OFF" : "ON"
  );
  send_debug("[DEBUG]       L1:%s L2:%s L3:%s L4:%s L5:%s L6:%s L7:%s L8:%s\n",
    (green & (1 << (1 - 1))) ? "OFF" : "ON",
    (green & (1 << (2 - 1))) ? "OFF" : "ON",
    (green & (1 << (3 - 1))) ? "OFF" : "ON",
    (green & (1 << (4 - 1))) ? "OFF" : "ON",
    (green & (1 << (5 - 1))) ? "OFF" : "ON",
    (green & (1 << (6 - 1))) ? "OFF" : "ON",
    (green & (1 << (7 - 1))) ? "OFF" : "ON",
    (green & (1 << (8 - 1))) ? "OFF" : "ON"
  );
  send_debug("\n");
}

void apply_traffic_state(uint8_t state, uint8_t &red, uint8_t &yellow, uint8_t &green) {
  switch (state) {
    case STATE_SYSTEM_INIT:
    case STATE_EVALUATE_QUEUE:
      red = 0x00;
      break;

    case STATE_LANE_1_GREEN:
      red = 0x00;
      set_light(red, 1, false);
      set_light(green, 1, true);
      break;

    case STATE_LANE_2_GREEN:
      red = 0x00;
      set_light(red, 2, false);
      set_light(green, 2, true);
      break;

    case STATE_LANE_3_GREEN:
      red = 0x00;
      set_light(red, 3, false);
      set_light(green, 3, true);
      break;

    case STATE_LANE_4_GREEN:
      red = 0x00;
      set_light(red, 4, false);
      set_light(green, 4, true);
      break;

    case STATE_LANE_5_GREEN:
      red = 0x00;
      set_light(red, 5, false);
      set_light(green, 5, true);
      break;

    case STATE_LANE_6_GREEN:
      red = 0x00;
      set_light(red, 6, false);
      set_light(green, 6, true);
      break;

    case STATE_LANE_7_GREEN:
      red = 0x00;
      set_light(red, 7, false);
      set_light(green, 7, true);
      break;

    case STATE_LANE_8_GREEN:
      red = 0x00;
      set_light(red, 8, false);
      set_light(green, 8, true);
      break;

    case STATE_APPROACH_1_2_GREEN:
      red = 0x00;
      set_light(red, 1, false);
      set_light(red, 2, false);
      set_light(green, 1, true);
      set_light(green, 2, true);
      break;

    case STATE_APPROACH_3_4_GREEN:
      red = 0x00;
      set_light(red, 3, false);
      set_light(red, 4, false);
      set_light(green, 3, true);
      set_light(green, 4, true);
      break;

    case STATE_APPROACH_5_6_GREEN:
      red = 0x00;
      set_light(red, 5, false);
      set_light(red, 6, false);
      set_light(green, 5, true);
      set_light(green, 6, true);
      break;

    case STATE_APPROACH_7_8_GREEN:
      red = 0x00;
      set_light(red, 7, false);
      set_light(red, 8, false);
      set_light(green, 7, true);
      set_light(green, 8, true);
      break;

    case STATE_OPPOSING_1_5_GREEN:
      // LHT: opposing straights (lanes 1+5: North+South) green; all right turns (2,4,6,8) red OFF (right-on-red)
      red = 0x00;
      set_light(red, 1, false);
      set_light(red, 5, false);
      set_light(red, 2, false);
      set_light(red, 4, false);
      set_light(red, 6, false);
      set_light(red, 8, false);
      set_light(green, 1, true);
      set_light(green, 5, true);
      break;

    case STATE_OPPOSING_3_7_GREEN:
      // LHT: opposing straights (lanes 3+7: East+West) green; all right turns (2,4,6,8) red OFF (right-on-red)
      red = 0x00;
      set_light(red, 3, false);
      set_light(red, 7, false);
      set_light(red, 2, false);
      set_light(red, 4, false);
      set_light(red, 6, false);
      set_light(red, 8, false);
      set_light(green, 3, true);
      set_light(green, 7, true);
      break;

    case STATE_YELLOW_TRANSITION:
      red = 0x00;
      yellow = 0x00;
      break;

    case STATE_ALL_RED_CLEARANCE:
      red = 0x00;
      break;

    case STATE_FLASHING_YELLOW:
      red = 0x00;
      yellow = 0x00;
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
      send_debug("[SLAVE] CMD_WRITE -> %s (0x%02X)\n", state_name(state), state);
      apply_state_to_registers(state);
      break;
    }

    case CMD_LOAD_CYCLE: {
      if (len < 2) return;
      uint8_t count = data[1];
      if (count == 0 || count > MAX_CYCLE_STEPS) {
        send_debug("[SLAVE] CMD_LOAD_CYCLE: invalid phase count\n");
        break;
      }
      uint8_t expected = 2 + count * 3; // CMD(1) + COUNT(1) + fase*3
      if (len < expected) {
        send_debug("[SLAVE] CMD_LOAD_CYCLE: payload too short\n");
        break;
      }

      for (uint8_t i = 0; i < count; i++) {
        nextCycle[i].phase_id = data[2 + i * 3];
        nextCycle[i].duration_sec = ((uint16_t)data[2 + i * 3 + 1] << 8) | data[2 + i * 3 + 2];
      }
      nextCycleCount = count;
      hasNewCycle = true;
      send_debug("[SLAVE] CMD_LOAD_CYCLE: %d phases staged (nextCycle)\n", count);

      if (fsmState == FSM_IDLE ||
          fsmState == FSM_FAILSAFE_RED ||
          fsmState == FSM_FAILSAFE_FLASHING) {
        activate_next_cycle();
        send_debug("[SLAVE] Cycle activated immediately\n");
      }
      break;
    }

    case CMD_FORCE_NEXT: {
      if (fsmState == FSM_GREEN) {
        apply_state_to_registers(STATE_YELLOW_TRANSITION);
        fsmState = FSM_YELLOW;
        fsmTimerStart = millis();
        send_debug("[SLAVE] CMD_FORCE_NEXT: GREEN -> YELLOW\n");
      } else if (fsmState == FSM_YELLOW) {
        apply_state_to_registers(STATE_ALL_RED_CLEARANCE);
        fsmState = FSM_RED;
        fsmTimerStart = millis();
        send_debug("[SLAVE] CMD_FORCE_NEXT: YELLOW -> RED\n");
      } else if (fsmState == FSM_RED) {
        start_next_phase_or_end();
        send_debug("[SLAVE] CMD_FORCE_NEXT: RED -> NEXT PHASE\n");
      } else {
        send_debug("[SLAVE] CMD_FORCE_NEXT: ignored (not in cycle)\n");
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
        send_debug("[SLAVE] CMD_EMERGENCY: all red, timer frozen\n");
      }
      break;
    }

    case CMD_RESUME: {
      if (fsmState == FSM_EMERGENCY) {
        if (currentCycleCount == 0) {
          fsmState = FSM_IDLE;
          send_debug("[SLAVE] CMD_RESUME: no cycle loaded, idle\n");
        } else {
          cycleElapsedStart = millis() - savedElapsedMs;
          start_green_phase();
          send_debug("[SLAVE] CMD_RESUME: cycle resumed\n");
        }
      } else {
        send_debug("[SLAVE] CMD_RESUME: not in emergency\n");
      }
      break;
    }

    default:
      send_debug("[SLAVE] Unknown COMMAND: 0x%02X\n", cmd);
      break;
  }
}