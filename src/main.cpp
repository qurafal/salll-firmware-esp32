#include "main.h"
#include "comm.h"


#define MAC_ADDRESS_INDEX 0


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

  init_esp_now(MAC_ADDRESS_INDEX);
  uint8_t data[2] = {10,10};
  esp_now_send(mac_addresses[1],data,2);
  send_debug("Halllo");

  update_shift_registers(0xFF, 0xFF, 0xFF);

}

void loop() {
  if (Serial.available()) {
    serial_recv(on_serial_recv_cb);
  }

  run_timer_fsm();
}