#include "comm.h"
#include <WiFi.h>
#include <esp_wifi.h>

 uint8_t mac_addresses[MAC_ADDRESS_TOTAL][MAC_LENGTH] = {
    {0xA4, 0x1A, 0xC4, 0x0A, 0x1A, 0x11},
    {0xA4, 0x1A, 0xC4, 0x0A, 0x1B, 0x11}
};

// static uint8_t debug_peer_mac[MAC_LENGTH] = {0};
static bool esp_now_ready = false;
static esp_now_peer_info_t peer_info;

static void on_esp_now_send(const uint8_t *mac_addr, esp_now_send_status_t status) {
  (void)mac_addr;
  (void)status;
}

static void on_data_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len) {
  (void)mac_addr;
  (void)data;
  (void)len;
}

static void on_data_sent_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
  (void)mac_addr;
  (void)status;
}

esp_err_t init_esp_now(uint8_t mac_index) {

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  esp_err_t result;

  result = esp_now_init();
  if (result != ESP_OK)
    return result;


  result = esp_now_register_recv_cb(on_data_recv_cb);
  if (result != ESP_OK)
        return result;

   uint8_t mac[MAC_LENGTH];
    memcpy(mac, mac_addresses[mac_index], MAC_LENGTH);
    result = esp_wifi_set_mac(WIFI_IF_STA, mac);
    if (result != ESP_OK)
        return result;

  memset(&peer_info, 0, sizeof(esp_now_peer_info_t));
    peer_info.channel = 0;
    peer_info.encrypt = false;

  /* Add All MAC to peer list  */
    for (int i = 0; i < MAC_ADDRESS_TOTAL; i++)
    {
        memcpy(peer_info.peer_addr, mac_addresses[i], MAC_LENGTH);
        result = esp_now_add_peer(&peer_info);
        if (result != ESP_OK)
            return result;
    }

    return ESP_OK;
}

void send_debug(const char *fmt, ...) {
  char buffer[256];
  va_list args;
  va_start(args, fmt);
  int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  if (len > 0 && esp_now_ready) {
    esp_now_send(mac_addresses[1], (const uint8_t *)buffer, len);
  }
}

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

    uint8_t *data_recv = nullptr;
    data_recv = new uint8_t[len];
    Serial.readBytes(data_recv, len);

    callback(data_recv, len);
}

int getMACIndex(const uint8_t *mac)
{
    for (int i = 0; i < MAC_ADDRESS_TOTAL; i++)
    {
        if (memcmp(mac, mac_addresses[i], MAC_LENGTH) == 0)
            return i;
    }
    return -1;
}