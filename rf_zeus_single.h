#pragma once
#include <Arduino.h>

extern void init_radio(unsigned int input_pin);

extern unsigned long wait_for_data();

extern void get_data(unsigned int* nbytes, byte* data);