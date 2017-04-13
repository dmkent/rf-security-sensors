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
  
//Define settings
#define MAX_SAMPLE 3000 // Maximum number of samples (transitions, 
                        // nominally two per bit) to record

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
   * Constructor. Takes receiver digital input pin as argument.
   */
  RF_ZEUS_RX(unsigned int input_pin);

  /*
   * Initialise the radio module.
   *
   * Should generally be called in setup().
   */
  void init();

  /*
   * Block until singal received.
   *
   * Calls yield() as appropriate to ensure watchdog timer stays happy.
   *
   * Buffers received data until receive is called.
   */
  void wait_until_avail();

  /*
   * Decode received signal and write resulting bytes back into buf.
   */
  void receive(unsigned int* nmessages, unsigned int* mes_len, byte* buf);

  /*
   * Do not call manually. Used internally to handle transition interrups.
   */
  void handle_interrupt();
  
  private:
  
  //Variables used in the ISR
  volatile unsigned long last = 0;
  volatile unsigned int count = 0;
  volatile unsigned long samples[MAX_SAMPLE];

  volatile boolean found_transition = false;
  volatile boolean is_high = true;
  volatile boolean new_state = true;

  int PREAMBLE_LEN = 25;
  int MESSAGE_LEN = 130;
  int MESSAGE_BYTE_LEN = 9;
  
  unsigned int _input_pin;

  /*
   * Get data pulse durations and store in samples.
   *
   * Gets all data pulses ready for decode. Will detect
   * the end of our 65bit word and receive the re-sync
   * signal before continuing.
   */
  void _get_data();

  void _print_data();
  
  void _send_data();

  /*
   * Method to block until we change state on input.
   *
   * Note the yield statement; the ESP8266 has a watchdog timer
   * that will reboot the system if we block too long. The yield
   * allows the system to go and do other things, such as maintain
   * wifi.
   */
   inline void _block_until_transition();

   bool _preamble_valid(int pos);

   bool _decode_message(int start, byte* message);

   void _send_message(byte* message);
};

/*
 * The ISR used to flag we've had a transition on the input.
 */
void transition();

#endif