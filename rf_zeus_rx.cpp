/*
 * Class to receive and decode signals from wireless sensors of an old
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

#include "rf_zeus_rx.h"

/*
 * Need to allow interrupt to call handler method.
 */
RF_ZEUS_RX* thisRx = nullptr;

RF_ZEUS_RX::RF_ZEUS_RX(unsigned int input_pin) {
  _input_pin = input_pin;
}

void RF_ZEUS_RX::init() {  
  pinMode(_input_pin, INPUT);
  thisRx = this;
}

void RF_ZEUS_RX::wait_until_avail() {
  attachInterrupt(_input_pin, transition, CHANGE);
  count = 0;
  _get_data();
  detachInterrupt(_input_pin);
}

void RF_ZEUS_RX::receive(unsigned int* nmessages, unsigned int* mes_len, byte* buf) {
  //print_data();
  //send_data();
  _decode_data();
}

/*
 * The ISR used to flag we've had a transition on the input.
 */
void RF_ZEUS_RX::handle_interrupt() {
  new_state = digitalRead(_input_pin);
  if (new_state != is_high) {
    found_transition = true;
    is_high = new_state;
  }
  
}

/*
 * Method to block until we change state on input.
 *
 * Note the yield statement; the ESP8266 has a watchdog timer
 * that will reboot the system if we block too long. The yield
 * allows the system to go and do other things, such as maintain
 * wifi.
 */
inline void RF_ZEUS_RX::_block_until_transition() {
    found_transition = false;
    while (!found_transition) {
      yield();
    }
}


/*
 * Get data pulse durations and store in samples.
 *
 * Gets all data pulses ready for decode. Will detect
 * the end of our 65bit word and receive the re-sync
 * signal before continuing.
 */
void RF_ZEUS_RX::_get_data(){
  count = 0;

  // Get initial signal
  int current, period = 0;
  int reps_required;
  boolean failed = false;

  while (true) {
    count = 0;
    last = micros();
    _block_until_transition();
    current = micros();
    period = current - last;
    samples[count++] = period;
    //if ((period < 19400) | (period > 19700)){
    if ((period < 16000) | (period > 21000)){
      continue;
    }
    last = current;

    reps_required = 10;
    
    _block_until_transition();
    current = micros();
    period = current - last;
    samples[count] = period;
    last = current;

    //Serial.println(period);
    if ((samples[count] > 1200) && (samples[count] < 1350)){
      // don't keep this record, but we don't fail yet
      // door sensors have this extra pulse.
    } else {
      // keep it and this means we need one less later
      reps_required--;
      count++;
    }
    
    failed = false;
    for(int i=0; i < reps_required; i++){
      _block_until_transition();
      current = micros();
      period = current - last;
      samples[count++] = period;
      //Serial.println(period);
      if ((period < 350) | (period > 600)){
        failed = true;
        break;
      }
      last = current;
    }

    if (! failed){
      break;
    }
  }

  bool skip = false;
  
  while (count < MAX_SAMPLE){
    _block_until_transition();
    current = micros();
    period = current - last;

    if (skip) {
      skip = false;
      continue;
    } else if (period < 250) {
      skip = true;
      continue;
    } else if (period > 100000) {
      break;
    }
    samples[count] = period;
    count++;
    last = current;
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
void RF_ZEUS_RX::_send_data() {
  byte current = 0;
  int pos = 0, bytecount = 0;
  Serial.println("start");
  // send count
  for (int i=0; i<4; ++i)
  {
    current = (byte)((count >> (8 * i)) & 0xff);
    Serial.write((int)current);
  }
  while (pos < count){

    for (int i=0; i<4; ++i)
    {
      current = (byte)((samples[pos] >> (8 * i)) & 0xff);
      Serial.write((int)current);
    }
    pos++;
    yield();
  }
  Serial.println("end");
}

bool RF_ZEUS_RX::_preamble_valid(int pos){
  return true;  
}

bool RF_ZEUS_RX::_decode_message(int start, byte* message){
  byte val;
  unsigned long left, right;
  int firsttrans;
  bool bitval;
  for(size_t ibyte=0; ibyte < MESSAGE_BYTE_LEN; ibyte++){
    val = 0;
    for(int ibit=0; ibit < 8; ibit++){
      firsttrans = 2 * ((8 * ibyte) + ibit);
      if ((firsttrans + 1) > MESSAGE_LEN) {
        break;
      }
      right = samples[start + firsttrans + 1];
      left = samples[start + firsttrans];
      if ((200 > left > 1000) || (200 > right > 1000)) {
        // period too short or too long.
        return false;
      }
      bitval = right > left;
      /*Serial.println(ibyte);
      Serial.println(ibit);
      Serial.println(left);
      Serial.println(right);
      Serial.println();*/
      val <<= 1;
      val += (int)bitval;
    }
    message[ibyte] = val;
  }
  return true;
}

void RF_ZEUS_RX::_send_message(byte* message) {
  Serial.println("start");
  // send count
  for (int i=0; i < MESSAGE_BYTE_LEN; ++i)
  {
    //Serial.println(i);
    Serial.print((int)message[i], HEX);
    //Serial.write((int)message[i]);
    yield();
  }
  Serial.println("");
  Serial.println("end");
}

void RF_ZEUS_RX::_decode_data() {
  int pos = 0;
  bool res = false;
  byte message[MESSAGE_BYTE_LEN];
  while(pos + PREAMBLE_LEN + MESSAGE_LEN < count) {
    if (_preamble_valid(pos)) {
      for(int i=0; i < MESSAGE_BYTE_LEN; i++){
        message[i] = 0;
      }
      res = _decode_message(pos + PREAMBLE_LEN, (byte*)&message);

      if (res) {
        _send_message((byte*)&message);
      }
    }
    pos += PREAMBLE_LEN + MESSAGE_LEN + 1;
  }
}

/*
 * ISR - proxies to handle_interrupt.
 */
void transition() {
  if (thisRx != nullptr)
    thisRx->handle_interrupt();
}

