#include "main.h"
#include "comm.h"

SystemState sysState = BOOT_CONFIG;
MicroFSM microFsms[NUM_STREAMS];
SlaveConfig slaveConfig;
uint8_t activeMask = 0;
unsigned long watchdogLastMs = 0;
bool inited = false;

void shiftOutByte(uint8_t dataPin, uint8_t clockPin, uint8_t val)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    digitalWrite(dataPin, (val & 0x80) ? HIGH : LOW);
    digitalWrite(clockPin, HIGH);
    delayMicroseconds(2);
    digitalWrite(clockPin, LOW);
    val <<= 1;
  }
}

void update_shift_registers(uint8_t byte_merah, uint8_t byte_kuning, uint8_t byte_hijau)
{
  digitalWrite(TRAFFIC_LATCH_PIN, LOW);
  shiftOutByte(TRAFFIC_DATA_PIN, TRAFFIC_CLOCK_PIN, byte_merah);
  shiftOutByte(TRAFFIC_DATA_PIN, TRAFFIC_CLOCK_PIN, byte_kuning);
  shiftOutByte(TRAFFIC_DATA_PIN, TRAFFIC_CLOCK_PIN, byte_hijau);
  digitalWrite(TRAFFIC_LATCH_PIN, HIGH);
}

void send_ack(uint8_t cmd)
{
  Serial.write(0xAB);
  Serial.write(0xCD);
  Serial.write(0xFF);
  Serial.write(0x01);
  Serial.write(cmd);
}

#define WATCHDOG_TIMEOUT_MS 5000UL

static void Task_FSM_Hardware(void *pv)
{
  for (;;)
  {
    uint8_t green = 0, yellow = 0, red = 0;
    for (int i = 0; i < NUM_STREAMS; i++)
    {
      switch (microFsms[i].state)
      {
      case GREEN:
        green |= (1 << i);
        break;
      case CLEARANCE_YELLOW:
        yellow |= (1 << i);
        break;
      case CLEARANCE_RED:
      case DISABLED_RED:
        red |= (1 << i);
        break;
      }
      // timers
      if (microFsms[i].state == CLEARANCE_YELLOW)
      {
        if (millis() - microFsms[i].timerStart >= (unsigned long)slaveConfig.y_sec * 1000UL)
        {
          microFsms[i].state = CLEARANCE_RED;
          microFsms[i].timerStart = millis();
        }
      }
      else if (microFsms[i].state == CLEARANCE_RED)
      {
        if (millis() - microFsms[i].timerStart >= (unsigned long)slaveConfig.r_sec * 1000UL)
        {
          microFsms[i].state = DISABLED_RED;
        }
      }
    }
    activeMask = green | yellow | red;
    activeMask = 0;
    for (int i = 0; i < NUM_STREAMS; i++)
      if (microFsms[i].state != DISABLED_RED)
        activeMask |= (1 << i);
    update_shift_registers(~red, ~yellow, ~green);

    static unsigned long lastDbg = 0;
    if (millis() - lastDbg > 1000)
    {
      const char *stateStr = "UNKNOWN";
      if (sysState == BOOT_CONFIG) stateStr = "BOOT_CONFIG";
      else if (sysState == RUNNING) stateStr = "RUNNING";
      else if (sysState == FALLBACK_CYCLE) stateStr = "FALLBACK_CYCLE";

      Serial.printf("[STATE] %s | R:%02X Y:%02X G:%02X | activeMask:0x%02X | Y:%ds R:%ds | HB:%lums ago\n",
                    stateStr, red, yellow, green, activeMask,
                    slaveConfig.y_sec, slaveConfig.r_sec,
                    sysState == RUNNING ? (millis() - watchdogLastMs) : 0);
      lastDbg = millis();
    }
    // watchdog
    if (sysState == RUNNING && millis() - watchdogLastMs > WATCHDOG_TIMEOUT_MS)
      sysState = FALLBACK_CYCLE;
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

static void Task_UART_Coordinator(void *pv)
{
  for (;;)
  {
    if (Serial.available())
      serial_recv(on_serial_recv_cb);
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void setup()
{
  delay(200);
  Serial.begin(115200);
  Serial.println("INIT-SALLL ESP32");
  pinMode(TRAFFIC_DATA_PIN, OUTPUT);
  pinMode(TRAFFIC_CLOCK_PIN, OUTPUT);
  pinMode(TRAFFIC_LATCH_PIN, OUTPUT);
  update_shift_registers(0xFF, 0xFF, 0xFF);
  for (int i = 0; i < NUM_STREAMS; i++)
  {
    microFsms[i].state = DISABLED_RED;
    microFsms[i].streamId = i;
  }
  xTaskCreate(Task_UART_Coordinator, "uart", 4096, NULL, 2, NULL);
  xTaskCreate(Task_FSM_Hardware, "fsm", 4096, NULL, 1, NULL);
}

void loop() { vTaskDelay(pdMS_TO_TICKS(1000)); }