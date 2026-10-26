#include "main.h"
#include "comm.h"

void on_serial_recv_cb(const uint8_t *data, int len) {
    send_debug("[SERIAL] RECV Data: ");
    for (int i = 0; i < len; i++) {
        send_debug("%02X ", data[i]);
    }
    send_debug("\n");
    processing_command((uint8_t *)data, len);
}