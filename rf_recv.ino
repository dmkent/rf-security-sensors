/*
 * Sketch to decode signals from wireless sensors of an old
 * security system.
 *
 * See the README.md for more details.
 *
 * Initially inspired by this forum post:
 *
 *   http://forum.arduino.cc/index.php?topic=242010.msg1738702#msg1738702
 *
 * Copyright 2017 David M Kent.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Define this to enable serial output
// #define DEBUG_RX

#include "rf_zeus_single.h"

#include "rf_config.h"

void setup() {
  init_radio(INPUT_PIN);
  Serial.begin(115200);
  while (!Serial) {yield();};
  Serial.println("\nRF decode sketch started");

  //pinMode(2, OUTPUT);
  //digitalWrite(2, HIGH);
}

void loop() {
  byte buf[9 * 100];
  unsigned int buflen = sizeof(buf);
  unsigned int nmessages = 0, mess_len = 9;
  
  Serial.println("Waiting for signal...");
  wait_for_data();
  
  //digitalWrite(2, LOW);
  get_data(&nmessages, buf);

  transmit_data(nmessages, mess_len, buf);
  
  //digitalWrite(2, HIGH);

  Serial.println("done.");
}

/*
 * Transmit decoded data, word by word.
 */
void transmit_data(unsigned int nmessages, unsigned int mess_len, byte* data) {
  byte current = 0, bitval = 0;
  int pos = 0, bytecount = 0;
  for(int imess=0; imess < nmessages; imess = imess + mess_len) {
    processWord(data + (imess * mess_len), mess_len);
  }
}

/*
 * Process transmission of a single message using MQTT client.
 *
 * Looks up each word in sensor_codes and sends "on" to appropriate
 * MQTT topic.
 */
void processWord(byte *data, unsigned int nbytes){
  char topic[50] = "";

  // Search configured sensors to see if it is registered
  for (int i=0; i < num_sensors; i++) {
    topic[0] = '\0';

    if(memcmp(data, sensor_codes[i].code, nbytes - 1) == 0) {
      // Found a match...
      strcat(topic, outTopic);
      strcat(topic, "/");
      strcat(topic, sensor_codes[i].topic);
      
      Serial.println(topic);
    }
  }
}
