#include "Joystick.h"

// This program uses a HK-T6A V2 or compatible RC transmitter connected to
// the RX pin as a USB HIB joystick.
// The bottom left hole on in the 4-pin DIN connector on the transmitter is the
// TX port. Connect this to the Arduino RX pin. Connect the connector shield to
// GND on the Arduino.

#define SERIAL_DEBUG 0

void setup() {
#if SERIAL_DEBUG
  Serial.begin(9600);
#endif
  Serial1.begin(115200);
  Joystick.begin(false);
}

/*
18 Bytes, TX->PC

Byte [0,1]: Header, always [0x55, 0xFC]
Byte [2-15]: Payload, Potmeter values.
[2] MSB [3] LSB CH1
[4] MSB [5] LSB CH2
[6] MSB [7] LSB CH3
[8] MSB [9] LSB CH4
[10] MSB [11] LSB CH5
[12] MSB [13] LSB CH6
[14] MSB [15] LSB CH4 but between 0 and 1000?
Byte [16,17]: Checksum. All bytes of payload added up, MSB first.
*/

// Length of POT messages, including the header
#define MSG_POT_LEN 18
#define MSG_START 0x55
#define MSG_TYPE_POT 0xFC

uint8_t msg_type = 0;
uint8_t bytesRead = 0;
uint8_t pot_msg_buf[MSG_POT_LEN];

// ch = 1,2,3,4,5,6
int16_t get_raw_channel_value(uint8_t ch) {
  if (ch < 1 || ch > 6) {
    return 0;
  }
  return (pot_msg_buf[2 * ch] << 8 | pot_msg_buf[2 * ch + 1]);
}

int16_t get_channel_value(uint8_t ch) {
  int16_t value = get_raw_channel_value(ch);
  return constrain(map(value, 1000, 2000, -127, 128), -127, 128);
}

void updatePotValues() {
#if SERIAL_DEBUG
  Serial.print("pot in: ");
  for (uint8_t ch = 1; ch <= 6; ch++) {
    Serial.print(get_raw_channel_value(ch));
    Serial.print(" ");
  }
  Serial.println();

  Serial.print("pot out: ");
  for (uint8_t ch = 1; ch <= 6; ch++) {
    Serial.print(get_channel_value(ch));
    Serial.print(" ");
  }
  Serial.println();
#endif

  Joystick.setXAxis(get_channel_value(1));
  Joystick.setYAxis(get_channel_value(2));
  Joystick.setThrottle(get_channel_value(3));
  Joystick.setRudder(get_channel_value(4));

  Joystick.setZAxis(get_channel_value(5));
  Joystick.setXAxisRotation(get_channel_value(6));

  Joystick.sendState();
}

void loop() {
  if (Serial1.available()) {
    uint8_t b = Serial1.read();
    if (bytesRead == 0) {
      // We're waiting for the next message
      if (b == MSG_START) {
        bytesRead = 1;
      }
    } else if (msg_type == 0) {
      // We are waiting for the message type
      if (b == MSG_TYPE_POT) {
        // We found a POT message
        msg_type = MSG_TYPE_POT;
        bytesRead = 2;
      } else {
        // Ignore the rest
        bytesRead = 0;
      }
    } else if (msg_type == MSG_TYPE_POT && bytesRead < MSG_POT_LEN) {
      // We are reading a message
      pot_msg_buf[bytesRead] = b;
      bytesRead++;
    }

    if (bytesRead == MSG_POT_LEN) {
      msg_type = 0;
      bytesRead = 0;
      updatePotValues();
    }

  }

}
