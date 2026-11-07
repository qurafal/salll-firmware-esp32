#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TRAFFIC_DATA_PIN  2
#define TRAFFIC_CLOCK_PIN 4
#define TRAFFIC_LATCH_PIN 16

#define NUM_STREAMS 8

enum SystemState : uint8_t { BOOT_CONFIG=0, RUNNING=1, FALLBACK_CYCLE=2 };
enum MicroFsmState : uint8_t { DISABLED_RED=0, GREEN=1, CLEARANCE_YELLOW=2, CLEARANCE_RED=3 };

struct MicroFSM {
  MicroFsmState state;
  unsigned long timerStart;
  uint8_t streamId;
};

struct SlaveConfig {
  uint8_t y_sec;
  uint8_t r_sec;
  uint8_t matrix[8];
  uint8_t fallbackData[64];
  uint8_t fallbackLen;
};

void set_light(uint8_t &byte, uint8_t lane_id, bool on);
void shiftOutByte(uint8_t dataPin, uint8_t clockPin, uint8_t val);
void update_shift_registers(uint8_t byte_merah, uint8_t byte_kuning, uint8_t byte_hijau);
void send_ack(uint8_t cmd);
void processing_command(uint8_t *data, int len);
void serial_recv(void (*callback)(const uint8_t *data, int len));
void on_serial_recv_cb(const uint8_t *data, int len);

extern SystemState sysState;
extern MicroFSM microFsms[NUM_STREAMS];
extern SlaveConfig slaveConfig;
extern uint8_t activeMask;
extern unsigned long watchdogLastMs;
extern bool inited;

#ifndef MAX_SERIAL_PAYLOAD
#define MAX_SERIAL_PAYLOAD 64
#endif

#endif