#include "main.h"

void on_serial_recv_cb(const uint8_t *data, int len) {
    Serial.printf("[SERIAL] RECV Data: ");
    for (int i = 0; i < len; i++) {
        Serial.printf("%02X ", data[i]);
    }
    Serial.println();
    processing_command((uint8_t *)data, len);
}
