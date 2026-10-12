#include "main.h"

#define SIMULATE_CYCLE_MS 3000UL

const uint8_t sim_sequence[] = {
  CMD_WRITE, STATE_NS_STRAIGHT,
  CMD_WRITE, STATE_YELLOW_TRANSITION,
  CMD_WRITE, STATE_ALL_RED_CLEARANCE,
  CMD_WRITE, STATE_WE_STRAIGHT,
  CMD_WRITE, STATE_YELLOW_TRANSITION,
  CMD_WRITE, STATE_ALL_RED_CLEARANCE,
  CMD_WRITE, STATE_N_PRIORITY,
  CMD_WRITE, STATE_YELLOW_TRANSITION,
  CMD_WRITE, STATE_ALL_RED_CLEARANCE,
  CMD_WRITE, STATE_S_PRIORITY,
  CMD_WRITE, STATE_YELLOW_TRANSITION,
  CMD_WRITE, STATE_ALL_RED_CLEARANCE,
  CMD_WRITE, STATE_W_PRIORITY,
  CMD_WRITE, STATE_YELLOW_TRANSITION,
  CMD_WRITE, STATE_ALL_RED_CLEARANCE,
  CMD_WRITE, STATE_E_PRIORITY,
  CMD_WRITE, STATE_YELLOW_TRANSITION,
  CMD_WRITE, STATE_ALL_RED_CLEARANCE,
  CMD_WRITE, STATE_FLASHING_YELLOW,
  CMD_WRITE, STATE_EMERGENCY,
  CMD_WRITE, STATE_SYSTEM_INIT
};

bool simulate_active = false;
size_t sim_index = 0;
unsigned long sim_last = 0;

void run_simulation_tick() {
  if (!simulate_active) return;
  unsigned long now = millis();
  if (now - sim_last >= SIMULATE_CYCLE_MS) {
    sim_last = now;
    uint8_t pkt[2];
    memcpy(pkt, &sim_sequence[sim_index], 2);
    sim_index = (sim_index + 2) % sizeof(sim_sequence);
    processing_command(pkt, 2);
  }
}
