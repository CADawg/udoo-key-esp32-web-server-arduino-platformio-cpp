//
// Created by conor on 28/09/23.
//

#ifndef ARDUINO_WEBSERVER_LEDHELPER_H
#define ARDUINO_WEBSERVER_LEDHELPER_H

#include <Arduino.h>

// Define constants
#define BLUE_LED_PIN GPIO_NUM_32
#define YELLOW_LED_PIN GPIO_NUM_33

// Function prototypes for the public functions in ledhelper.cpp
void turnOnLedForMS(int pin, int64_t ms);
void turnOnLed(int pin);
void ledTick();

#endif // ARDUINO_WEBSERVER_LEDHELPER_H
