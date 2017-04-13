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

#include "rf_zeus_rx.h"
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>

#include "rf_config.h"

WiFiClient espClient;
PubSubClient client(espClient);
Ticker ticker;
RF_ZEUS_RX radio(INPUT_PIN);

void setup() {
  wifi_down();

  radio.init();
#ifdef DEBUG_RX
  Serial.begin(115200);
  while (!Serial) {yield();};
  Serial.println("\nRF probe sketch started");
#endif

  client.setServer(mqtt_server, 1883);

  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
}

void wifi_down() {
  WiFi.forceSleepBegin();          // turn off ESP8266 RF
  delay(1);                        // give RF section time to shutdown
}

void wifi_up() {
  delay(10);
  // We start by connecting to a WiFi network
  WiFi.forceSleepWake();
  WiFi.mode(WIFI_STA);  
  //wifi_station_connect();
  delay(1);

#ifdef DEBUG_RX
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
#endif

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#ifdef DEBUG_RX
    Serial.print(".");
#endif
  }

#ifdef DEBUG_RX
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
#endif

  // Now get MQTT client connected to server
  // Loop until we're reconnected
  while (!client.connected()) {
#ifdef DEBUG_RX
    Serial.print("Attempting MQTT connection...");
#endif
    // Attempt to connect
    if (client.connect(clientID)) {
#ifdef DEBUG_RX
      Serial.println("connected");
#endif
    } else {
#ifdef DEBUG_RX
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
#endif
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  byte buf[9 * 100];
  unsigned int buflen = sizeof(buf);
  unsigned int nmessages = 0, mess_len = 0;
  
  wifi_down();
#ifdef DEBUG_RX
  Serial.println("Waiting for signal...");
#endif
  radio.wait_until_avail();
  
  digitalWrite(2, LOW);
  radio.receive(&nmessages, &mess_len, buf);

  wifi_up();
  
  transmit_data(nmessages, mess_len, buf);
  
  digitalWrite(2, HIGH);

  // let everything catch up
  for (int j=0; j< 4; j++) {
    delay(500);
    client.loop();
    yield();
  }

#ifdef DEBUG_RX
  Serial.println("done.");
#endif
  client.disconnect();
}

/*
 * Transmit decoded data, word by word.
 */
void transmit_data(unsigned int nmessages, unsigned int mess_len, byte* data) {
  byte current = 0, bitval = 0;
  int pos = 0, bytecount = 0;
  for(int imess=0; imess < nmessages; imess++) {
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
      
      client.publish(topic, "on");
    }
  }
}