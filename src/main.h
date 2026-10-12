#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>

#define TRAFFIC_DATA_PIN  2
#define TRAFFIC_CLOCK_PIN 4
#define TRAFFIC_LATCH_PIN 16

#define LANE_N_S  0
#define LANE_N_R  1
#define LANE_S_S  2
#define LANE_S_R  3
#define LANE_E_S  4
#define LANE_E_R  5
#define LANE_W_S  6
#define LANE_W_R  7

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
  CMD_UTILS = 0x02,
  CMD_LOAD_CYCLE = 0x03,
  CMD_FORCE_NEXT = 0x04,
  CMD_EMERGENCY = 0x05,
  CMD_RESUME = 0x06,
  CMD_CYCLE_DONE = 0x07,
  CMD_REQ_CYCLE = 0x08
};

enum UtilsCmd : uint8_t {
  UTILS_HELP = 0x01,
  UTILS_SIMULATE_ON = 0x02,
  UTILS_SIMULATE_OFF = 0x03
};

enum TrafficState : uint8_t {
  STATE_SYSTEM_INIT       = 0,
  STATE_EVALUATE_QUEUE    = 1,
  STATE_NS_STRAIGHT       = 10,
  STATE_N_PRIORITY        = 11,
  STATE_S_PRIORITY        = 12,
  STATE_WE_STRAIGHT       = 13,
  STATE_W_PRIORITY        = 14,
  STATE_E_PRIORITY        = 15,
  STATE_YELLOW_TRANSITION = 20,
  STATE_ALL_RED_CLEARANCE = 21,
  STATE_FLASHING_YELLOW   = 22,
  STATE_EMERGENCY         = 23
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

inline void set_light(uint8_t &byte, uint8_t lane, bool on);
void shiftOutByte(uint8_t dataPin, uint8_t clockPin, uint8_t val);
void update_shift_registers(uint8_t byte_merah, uint8_t byte_kuning, uint8_t byte_hijau);
const char* state_name(uint8_t state);
void print_debug(uint8_t state, uint8_t red, uint8_t yellow, uint8_t green);
void apply_traffic_state(uint8_t state, uint8_t &red, uint8_t &yellow, uint8_t &green);
void apply_state_to_registers(uint8_t state, bool debug = true);
void processing_command(uint8_t *data, int len);
void serial_recv(void (*callback)(const uint8_t *data, int len));
void on_serial_recv_cb(const uint8_t *data, int len);

// legacy simulation module
extern bool simulate_active;
extern size_t sim_index;
extern unsigned long sim_last;
void run_simulation_tick();

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
