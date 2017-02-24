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
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

#include "rf_config.h";

//Define settings
const unsigned int MAX_SAMPLE = 1024; //Maximum number of samples to record
const unsigned int WORD_BYTES = 8;

//Variables used in the ISR
volatile boolean running = false;
volatile unsigned long last = 0;
volatile unsigned int count = 0;
volatile unsigned long samples[MAX_SAMPLE];
volatile boolean found_transition = false;

WiFiClient espClient;
PubSubClient client(espClient);
char msg[WORD_BYTES * 3];

void setup() {
  pinMode(INPUT_PIN, INPUT);
  Serial.begin(9600);
  while (!Serial) {yield();};
  Serial.println("\nRF decode sketch started");

  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientID)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  attachInterrupt(INPUT_PIN, transition, CHANGE);
  sync();
  count = 0;
  get_data();
  detachInterrupt(INPUT_PIN);
  //print_data();

  if (!client.connected()) {
    reconnect();
  }

  decode_data();

  client.loop();
}

/*
 * The ISR used to flag we've had a transition on the input.
 */
void transition() {
  found_transition = true;
}

/*
 * Method to block until we change state on input.
 *
 * Note the yield statement; the ESP8266 has a watchdog timer
 * that will reboot the system if we block too long. The yield
 * allows the system to go and do other things, such as maintain
 * wifi.
 */
inline void block_until_transition() {
    found_transition = false;
    while (!found_transition) {
      yield();
    }
}

/*
 * Look for a "resync" signal on the input.
 *
 * Returns true if one found, false otherwise.
 *
 * Signal expected is 11 pairs of pulses followed
 * by one single longer pair.
 */
boolean get_resync_signal() {
    int current = 0, period = 0;
    boolean failed = false;

    for(int i=0; i <= 10; i++){
      block_until_transition();
      current = micros();
      period = current - last;
      if ((period < 300) | (period > 420)){
        failed = true;
        break;
      }
      last = current;

      block_until_transition();
      current = micros();
      period = current - last;
      if ((period < 370) | (period > 480)){
        failed = true;
        break;
      }
      last = current;
    }

    if (failed) {
      return true;
    }

    block_until_transition();
    current = micros();
    period = current - last;
    if ((period < 300) | (period > 420)){
      return true;
    }
    last = current;

    block_until_transition();
    current = micros();
    period = current - last;
    if ((period < 3700) | (period > 4100)){
      return true;
    }
    last = current;

    return failed;
}

/*
 * Block until a full "sync"/preamble signal is detected.
 *
 * This function will block (apart for calls to yield()) until
 * we detect a full signal preamble. This consists of one long pulse
 * (around 17ms) followed by a 1250us pulse and a resync sequence.
 *
 * When this method returns we are ready to receive data.
 */
void sync(){
  int current, period = 0;
  boolean failed = false;

  while (true) {
    last = micros();
    block_until_transition();
    current = micros();
    period = current - last;
    if ((period < 16000) | (period > 18000)){
      continue;
    }
    last = current;

    block_until_transition();
    current = micros();
    period = current - last;
    if ((period < 1200) | (period > 1350)){
      continue;
    }
    last = current;

    failed = get_resync_signal();
    if (failed) {
      continue;
    }

    break;
  }
}

/*
 * Get data pulse durations and store in samples.
 *
 * Gets all data pulses ready for decode. Will detect
 * the end of our 65bit word and receive the re-sync
 * signal before continuing.
 */
void get_data(){
  boolean failed_resync;
  last = micros();
  count = 0;
  while (count < MAX_SAMPLE){
    block_until_transition();
    samples[count] = micros() - last;

    if ((samples[count] > 14500) & (samples[count] < 15500)) {
      // We just recorded second pulse of two pulse resync signal
      // get rest.
      last = micros();
      failed_resync = get_resync_signal();

      if (failed_resync) {
        Serial.println("bailing");
        return;
      } else {
        count--;
        last = micros();
        continue;
      }
    } else if (samples[count] > 200000) {
      return;
    }
    last = micros();
    count++;
  }
}

/*
 * Decode data in samples.
 *
 * Applies simple on-off-keying (OOK) algorithm to
 * decode 64 bit sequences from samples.
 * Currently ignores 65th bit (parity bit?)
 *
 * Currently just prints decoded data to serial.
 * TODO: do something with decoded data (send via
 *       MQTT probably.
 */
void decode_data() {
  byte data[WORD_BYTES];
  byte current = 0, bitval = 0;
  int pos = 0, bytecount = 0;
  while (pos + 1 < count){
    current = 0;
    for(int bitcount=0; bitcount < 8; bitcount++){
      if(samples[pos] > samples[pos + 1]){
        bitval = 0;
      } else {
        bitval = 1;
      }
      current <<= 1;
      current += bitval;
      pos += 2;
    }
    data[bytecount] = current;
    sprintf(msg + (bytecount * 2), "%02x", current);

    bytecount++;

    if (bytecount >= WORD_BYTES) {
      // End of word
      client.publish(outTopic, msg);
      for (int i=0; i< bytecount; i++){
        Serial.println(data[i], HEX);
      }
      Serial.println("");
      // skip checksum bit
      pos += 2;
      bytecount = 0;
    }
  }

}

/*
 * Debug function, prints pulse lengths to serial.
 */
void print_data(){
  int startState = 0;
  Serial.print("I recorded ");
  Serial.print(count);
  Serial.println(" samples");
  for (unsigned int x = 0; x < count; x++) {
    Serial.print("Pin = ");
    if (startState == 0) {
      Serial.print("Low ");
    } else {
      Serial.print("High");
    }
    startState = !startState;
    Serial.print(" for ");
    Serial.print(samples[x]);
    Serial.println(" usec");
    yield();
  }

  Serial.println("");
}
