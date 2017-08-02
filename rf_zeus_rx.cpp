/*
 * Class to receive and decode signals from wireless sensors of an old
 * security system.
 *
 * See the README.md for more details.
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

#include "rf_zeus_single.h"
#include "rf_zeus_rx.h"
#include "src/ZeusRfDecode.h"

void RF_ZEUS_RX::init(unsigned int input_pin) {
  _input_pin = input_pin;
  pinMode(_input_pin, INPUT);
}

void RF_ZEUS_RX::set_buffer(byte* buffer) {
  this->_buffer = buffer;
  count = 0;
}

unsigned int RF_ZEUS_RX::get_byte_count() {
  return count;
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
 * Get pair of transitions and store in passed pointers.
 */
void RF_ZEUS_RX::get_transition_pair(unsigned long* pos_start, unsigned long* pos_end,
                                     unsigned long* width_high, unsigned long* width_low){
  unsigned long middle;
  (*pos_start) = micros();
  _block_until_transition();
  middle = micros();
  (*width_high) = middle - (*pos_start);
  _block_until_transition();
  (*pos_end) = micros();
  (*width_low) = (*pos_end) - middle;

}

/*
 * Advance position until we hit a HIGH pulse.
 */
void RF_ZEUS_RX::advance_to_high_pulse(){
  if (!is_high) {
    _block_until_transition();
  }
}

/*
 * Handle recieve of sync bit
 */
void RF_ZEUS_RX::handle_sync_bit(unsigned long pos) {
  return;
}

/*
 * Handle a completed decoded data byte
 */
void RF_ZEUS_RX::handle_data_byte(unsigned long byte_start, unsigned long byte_end, byte data) {
  _buffer[count++] = data;
}

/*
 * Need to allow interrupt to call handler method.
 */
RF_ZEUS_RX radio;

/*
 * ISR - proxies to handle_interrupt.
 */
void transition() {
    radio.handle_interrupt();
}

void _single_advance_to_high() {
    radio.advance_to_high_pulse();
}

void _single_get_pair(unsigned long* pos_start, unsigned long* pos_end,
                      unsigned long* width_high, unsigned long* width_low) {
    radio.get_transition_pair(pos_start, pos_end, width_high, width_low);
}

void _single_mark_byte(unsigned long byte_start, unsigned long byte_end, byte data) {
    radio.handle_data_byte(byte_start, byte_end, data);
}

void _single_mark_sync(unsigned long pos){
    return;
}

void init_radio(unsigned int input_pin){
    radio.init(input_pin);
}

unsigned long wait_for_data(){
    return block_until_data(
        &_single_advance_to_high,
        &_single_get_pair,
        &_single_mark_sync
    );
}

void get_data(unsigned int* nbytes, byte* data){
    radio.set_buffer(data);
    receive_and_process_data(
        0,
        &_single_get_pair,
        &_single_mark_byte
    );
    (*nbytes) = radio.get_byte_count();
}
