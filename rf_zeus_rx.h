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
#ifndef RF_ZEUS_RX_h
#define RF_ZEUS_RX_h

#include <Arduino.h>

/*
 * Radio access class. Provides functionality to block until we receive a valid
 * signal then a method to read the resulting payload bytes out.
 *
 * Note: this relies heavily on near real-time interrupts on pin transition. As a
 *       result this generally needs the WiFi module powered down to achieve
 *       reasonable performance. This can be done with WiFi.forceSleepBegin()
 */
class RF_ZEUS_RX {
  public:
  /*
   * Initialise the radio module.
   *
   * Should generally be called in setup().
   */
  void init(unsigned int input_pin);

  void set_buffer(byte* buffer);

  unsigned int get_byte_count();

  /*
   * Do not call manually. Used internally to handle transition interrups.
   */
  void handle_interrupt();

  void get_transition_pair(unsigned long* pos_start, unsigned long* pos_end,
                           unsigned long* width_high, unsigned long* width_low);

  /*
   * Advance position until we hit a HIGH pulse.
   */
  void advance_to_high_pulse();

  /*
   * Handle recieve of sync bit
   */
  void handle_sync_bit(unsigned long pos);

  /*
   * Handle a completed decoded data byte
   */
  void handle_data_byte(unsigned long byte_start, unsigned long byte_end, byte data);

  private:

  int PREAMBLE_LEN = 25;
  int MESSAGE_LEN = 130;
  int MESSAGE_BYTE_LEN = 9;

  unsigned int count = 0;
  byte* _buffer;

  //Variables used in the ISR
  volatile boolean found_transition = false;
  volatile boolean is_high = true;
  volatile boolean new_state = true;

  unsigned int _input_pin;

  /*
   * Method to block until we change state on input.
   *
   * Note the yield statement; the ESP8266 has a watchdog timer
   * that will reboot the system if we block too long. The yield
   * allows the system to go and do other things, such as maintain
   * wifi.
   */
   inline void _block_until_transition();
};

/*
 * The ISR used to flag we've had a transition on the input.
 */
void transition();

#endif
