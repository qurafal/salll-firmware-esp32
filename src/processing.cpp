#include "main.h"
#include "comm.h"

#define CMD_INIT 0x10
#define CMD_SET_STREAM 0x30
#define CMD_HEARTBEAT 0x40
#define CMD_RESYNC 0x50
#define ACK_INIT_OK 0x11
#define ACK_TRANSITION 0x31
#define ACK_REJECTED 0xFF

void processing_command(uint8_t *data, int len) {
  if (len < 1) return;
  uint8_t cmd = data[0];
  if (sysState==BOOT_CONFIG) {
    if (cmd==CMD_INIT) {
      if (len < 11) return;
      slaveConfig.y_sec = data[1];
      slaveConfig.r_sec = data[2];
      for (int i=0;i<8;i++) slaveConfig.matrix[i]=data[3+i];
      slaveConfig.fallbackLen = len-11;
      for (int i=0;i<slaveConfig.fallbackLen;i++) slaveConfig.fallbackData[i]=data[11+i];
      sysState=RUNNING;
      watchdogLastMs=millis();
      inited=true;
      send_ack(ACK_INIT_OK);
    }
    return;
  }
  if (sysState==RUNNING) {
    if (cmd==CMD_HEARTBEAT) { watchdogLastMs=millis(); return; }
    if (cmd==CMD_SET_STREAM) {
      if (len<3) return;
      uint8_t sid=data[1]; uint8_t tgt=data[2];
      if (sid>=NUM_STREAMS) { send_ack(ACK_REJECTED); return; }
      if (tgt==1) {
        uint8_t conflict = activeMask & slaveConfig.matrix[sid];
        if (conflict) { send_ack(ACK_REJECTED); return; }
        microFsms[sid].state=GREEN; microFsms[sid].timerStart=millis();
        send_ack(ACK_TRANSITION);
      } else {
        if (microFsms[sid].state==GREEN){ microFsms[sid].state=CLEARANCE_YELLOW; microFsms[sid].timerStart=millis(); send_ack(ACK_TRANSITION);}
        else send_ack(ACK_REJECTED);
      }
      return;
    }
    return;
  }
  if (sysState==FALLBACK_CYCLE) {
    if (cmd==CMD_RESYNC) {
      for (int i=0;i<NUM_STREAMS;i++) microFsms[i].state=DISABLED_RED;
      update_shift_registers(0xFF,0xFF,0xFF);
      sysState=BOOT_CONFIG;
    }
    return;
  }
}

void on_serial_recv_cb(const uint8_t *data, int len) {
  processing_command((uint8_t*)data, len);
  delete[] data;
}