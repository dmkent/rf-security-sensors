/*
 * Very simple external interface for rf_zeus_rx.
 *
 * Exposes only c-style functions, ready for use in the sketch.
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
#pragma once
#include <Arduino.h>

/*
 * Initialise the singleton radio class to use given input_pin
 *
 * Must be called before any of the other methods.
 */
extern void init_radio(unsigned int input_pin);

/*
 * Block until data preamble is detected.
 */
extern unsigned long wait_for_data();

/*
 * Read data as bytes into data array.
 */
extern void get_data(unsigned int* nbytes, byte* data);