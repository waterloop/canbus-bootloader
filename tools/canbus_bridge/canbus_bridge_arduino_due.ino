#include "variant.h"
#include <due_can.h>

#define Serial SerialUSB

void onFrame(CAN_FRAME *frame) {
  uint8_t pkt[16];
  *(uint32_t *) pkt = frame->id;
  pkt[4] = frame->extended;
  pkt[5] = frame->length;
  *(uint32_t *) (pkt + 8) = frame->data.low;
  *(uint32_t *) (pkt + 12) = frame->data.high;
  Serial.write(pkt, 16);
}

void setup()
{

  Serial.begin(115200);
  
  Can0.begin(CAN_BPS_1000K);

  Can0.setRXFilter(0, 0, 0, true);
  Can0.setRXFilter(1, 0, 0, false);

  Can0.setCallback(0, onFrame);
  Can0.setCallback(1, onFrame);
}

CAN_FRAME toSend;
uint8_t readBuffer[16];

void loop(){
  if(Serial.readBytes(readBuffer, 16) == 16) {
    toSend.id = (*(uint32_t *) readBuffer);
    toSend.extended = readBuffer[4];
    toSend.length = readBuffer[5];
    toSend.data.low = *(uint32_t *) (readBuffer + 8);
    toSend.data.high = *(uint32_t *) (readBuffer + 12);
    Can0.sendFrame(toSend);
  }
}
