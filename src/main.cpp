#include "main.h"

inline void set_light(uint8_t &byte, uint8_t lane, bool on) {
  if (on) {
    byte &= ~(1 << lane);
  } else {
    byte |= (1 << lane);
  }
}

void shiftOutByte(uint8_t dataPin, uint8_t clockPin, uint8_t val) {
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(dataPin, (val & 0x80) ? HIGH : LOW);
    digitalWrite(clockPin, HIGH);
    delayMicroseconds(2);
    digitalWrite(clockPin, LOW);
    val <<= 1;
  }
}

void update_shift_registers(uint8_t byte_merah, uint8_t byte_kuning, uint8_t byte_hijau) {
  digitalWrite(TRAFFIC_LATCH_PIN, LOW);
  shiftOutByte(TRAFFIC_DATA_PIN, TRAFFIC_CLOCK_PIN, byte_merah);
  shiftOutByte(TRAFFIC_DATA_PIN, TRAFFIC_CLOCK_PIN, byte_kuning);
  shiftOutByte(TRAFFIC_DATA_PIN, TRAFFIC_CLOCK_PIN, byte_hijau);
  digitalWrite(TRAFFIC_LATCH_PIN, HIGH);
}

static uint8_t serial_rx_buf[MAX_SERIAL_PAYLOAD];

void serial_recv(void (*callback)(const uint8_t *data, int len)) {
    if (Serial.available() < 5) return;

    uint8_t h1 = Serial.read();
    if (h1 != 0xAB) return;

    uint8_t h2 = Serial.read();
    if (h2 != 0xCD) return;

    uint8_t h3 = Serial.read();
    if (h3 != 0xFF) return;

    uint8_t len = Serial.read();
    if (len == 0 || len > MAX_SERIAL_PAYLOAD) return;

    if (Serial.available() < len) return;

    for (uint8_t i = 0; i < len; i++) {
        serial_rx_buf[i] = Serial.read();
    }

    callback(serial_rx_buf, len);
}

void setup() {
  delay(200);

  Serial.begin(115200);

  pinMode(TRAFFIC_DATA_PIN, OUTPUT);
  pinMode(TRAFFIC_CLOCK_PIN, OUTPUT);
  pinMode(TRAFFIC_LATCH_PIN, OUTPUT);

  digitalWrite(TRAFFIC_DATA_PIN, LOW);
  digitalWrite(TRAFFIC_CLOCK_PIN, LOW);
  digitalWrite(TRAFFIC_LATCH_PIN, LOW);

  delay(100);

  update_shift_registers(0xFF, 0xFF, 0xFF);
  Serial.println("[SLAVE] Smart Traffic Light Slave Executor Ready");
  Serial.println("[SLAVE] Protocol: AB CD FF <len> <cmd> <data...>");
  Serial.println("[SLAVE] FSM: PULL-BASED TIMING PLAN (shadow buffer + prefetch + failsafe)");
}

void loop() {
  if (Serial.available()) {
    serial_recv(on_serial_recv_cb);
  }

  run_timer_fsm();
  run_simulation_tick();
}
