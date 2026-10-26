#ifndef COMM_H
#define COMM_H

#include <Arduino.h>
#include <esp_now.h>

#define MAX_SERIAL_PAYLOAD 64
#define MAC_ADDRESS_TOTAL 2
#define MAC_LENGTH 6

extern uint8_t mac_addresses[MAC_ADDRESS_TOTAL][MAC_LENGTH];

esp_err_t init_esp_now(uint8_t);
void send_debug(const char *fmt, ...);
#endif