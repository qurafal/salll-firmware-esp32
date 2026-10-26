#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>

#define TRAFFIC_DATA_PIN  2
#define TRAFFIC_CLOCK_PIN 4
#define TRAFFIC_LATCH_PIN 16

// Lane IDs are 1..8 (used directly in set_light / apply_traffic_state).
// No #define constants for individual lanes — use literal IDs.
// Bit position = lane_id - 1.
//
// LHT (Indonesia) physical mapping (clockwise N,E,S,W):
//   Lane 1 = North Straight    Lane 5 = South Straight
//   Lane 2 = North Right Turn  Lane 6 = South Right Turn
//   Lane 3 = East Straight     Lane 7 = West Straight
//   Lane 4 = East Right Turn   Lane 8 = West Right Turn

#define MAX_CYCLE_STEPS 8

// FSM timing constants (milliseconds)
#define FSM_YELLOW_DURATION_MS      3000UL
#define FSM_RED_DURATION_MS         2000UL
#define PREFETCH_THRESHOLD_MS       10000UL
#define FAILSAFE_RED_DURATION_MS    5000UL
#define FAILSAFE_FLASH_INTERVAL_MS  500UL
#define FAILSAFE_REQ_INTERVAL_MS    2000UL

enum Command : uint8_t {
  CMD_WRITE = 0x01,
  CMD_LOAD_CYCLE = 0x03,
  CMD_FORCE_NEXT = 0x04,
  CMD_EMERGENCY = 0x05,
  CMD_RESUME = 0x06,
  CMD_CYCLE_DONE = 0x07,
  CMD_REQ_CYCLE = 0x08
};

enum TrafficState : uint8_t {
  STATE_SYSTEM_INIT       = 0x00,
  STATE_EVALUATE_QUEUE    = 0x01,

  /* 8 individual lane green states (8-junction mode) */
  STATE_LANE_1_GREEN      = 0x10,
  STATE_LANE_2_GREEN      = 0x11,
  STATE_LANE_3_GREEN      = 0x12,
  STATE_LANE_4_GREEN      = 0x13,
  STATE_LANE_5_GREEN      = 0x14,
  STATE_LANE_6_GREEN      = 0x15,
  STATE_LANE_7_GREEN      = 0x16,
  STATE_LANE_8_GREEN      = 0x17,

  /* 4 approach pair green states (4-junction mode: straight + right) */
  STATE_APPROACH_1_2_GREEN = 0x18,
  STATE_APPROACH_3_4_GREEN = 0x19,
  STATE_APPROACH_5_6_GREEN = 0x1A,
  STATE_APPROACH_7_8_GREEN = 0x1B,

  /* 2 opposing pair green states (LHT: right-turn-on-red for all right lanes) */
  STATE_OPPOSING_1_5_GREEN = 0x1C,
  STATE_OPPOSING_3_7_GREEN = 0x1D,

  /* transition / system states */
  STATE_YELLOW_TRANSITION = 0x20,
  STATE_ALL_RED_CLEARANCE = 0x21,
  STATE_FLASHING_YELLOW   = 0x22,
  STATE_EMERGENCY         = 0x23
};

enum FsmState : uint8_t {
  FSM_IDLE = 0,
  FSM_GREEN = 1,
  FSM_YELLOW = 2,
  FSM_RED = 3,
  FSM_FAILSAFE_RED = 4,
  FSM_FAILSAFE_FLASHING = 5,
  FSM_EMERGENCY = 6
};

struct CycleStep {
  uint8_t phase_id;
  uint16_t duration_sec;
};

inline void set_light(uint8_t &byte, uint8_t lane_id, bool on) {
  uint8_t bit = lane_id - 1;
  if (on) {
    byte &= ~(1 << bit);
  } else {
    byte |= (1 << bit);
  }
}
void shiftOutByte(uint8_t dataPin, uint8_t clockPin, uint8_t val);
void update_shift_registers(uint8_t byte_merah, uint8_t byte_kuning, uint8_t byte_hijau);
const char* state_name(uint8_t state);
void print_debug(uint8_t state, uint8_t red, uint8_t yellow, uint8_t green);
void apply_traffic_state(uint8_t state, uint8_t &red, uint8_t &yellow, uint8_t &green);
void apply_state_to_registers(uint8_t state, bool debug = true);
void processing_command(uint8_t *data, int len);
void serial_recv(void (*callback)(const uint8_t *data, int len));
void on_serial_recv_cb(const uint8_t *data, int len);

// pull-based timing plan FSM
void run_timer_fsm();
void send_req_cycle();
void send_cycle_done();
uint32_t compute_cycle_duration_ms(struct CycleStep *steps, uint8_t count);
void start_green_phase();
void start_next_phase_or_end();
void activate_next_cycle();

extern struct CycleStep currentCycle[MAX_CYCLE_STEPS];
extern struct CycleStep nextCycle[MAX_CYCLE_STEPS];
extern uint8_t currentCycleCount;
extern uint8_t nextCycleCount;
extern bool hasNewCycle;
extern uint8_t cycleIndex;
extern enum FsmState fsmState;
extern unsigned long fsmTimerStart;
extern unsigned long cycleElapsedStart;
extern uint32_t cycleTotalDurationMs;
extern bool reqCycleSent;
extern bool reqSentInFailsafeRed;
extern unsigned long failsafeLastReq;
extern bool flashingState;
extern uint32_t savedElapsedMs;

#define MAX_SERIAL_PAYLOAD 64

#endif
