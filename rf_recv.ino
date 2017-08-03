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
#define BAUD_RATE 9600
#define NMESS_BYTES 8
typedef struct {
  unsigned char code[NMESS_BYTES];  // The 8 byte sensor code
  const char* topic;            // The topic to identify the sensor
} SENSOR_IDENT;

#include "rf_zeus_rx.h"
#include "rf_config.h"


void setup() {
  // Set up serial. We will only write resulting data to serial.
  Serial.begin(BAUD_RATE);
  while (!Serial) {yield();};
  
  // Initialise the decoder.
  init_radio(INPUT_PIN);
  
  // Set up LED for recieve status 
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  byte buf[NMESS_BYTES + 20];
  unsigned int nrecv = 0;
  
  // Wait for some data to arrive (actually waits for preamble to finish)
  wait_for_data();
  
  // About to receive, turn LED on
  digitalWrite(LED_PIN, HIGH);

  // Get the incoming data, bytes get written to buf.
  get_data(&nrecv, buf);

  if (nrecv >= NMESS_BYTES) {
    // Received a full message (NMESS_BYTES)
    decode_message(buf);
  }

  digitalWrite(LED_PIN, LOW);
}


/*
 * Decode message by looking up in configured sensor_codes.
 *
 * Prints found sensor_code to Serial if found.
 *
 */
void decode_message(byte *data){
  char topic[50] = "";

  // Search configured sensors to see if it is registered
  for (int i=0; i < num_sensors; i++) {
    topic[0] = '\0';

    if(memcmp(data, sensor_codes[i].code, NMESS_BYTES) == 0) {
      // Found a match...
      strcat(topic, outTopic);
      strcat(topic, "/");
      strcat(topic, sensor_codes[i].topic);
      
      Serial.println(topic);
    }
  }
}
