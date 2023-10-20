//
// Created by conor on 28/09/23.
//
#include <Arduino.h>
#include <ledhelper.h>

bool isYellowLedOn = false;
int64_t yellowLedOffTime = 0;
bool isBlueLedOn = false;
int64_t blueLedOffTime = 0;

void turnOnLedForMS(int pin, int64_t ms) {
    if (pin == YELLOW_LED_PIN) {
        if (isYellowLedOn) {
            return;
        }
        digitalWrite(pin, HIGH);

        isYellowLedOn = true;
        yellowLedOffTime = millis() + ms;
    } else if (pin == BLUE_LED_PIN) {
        if (isBlueLedOn) {
            return;
        }
        digitalWrite(pin, HIGH);

        isBlueLedOn = true;
        blueLedOffTime = millis() + ms;
    }
}

void turnOnLed(int pin) {
    turnOnLedForMS(pin, 0);
}

void ledTick() {
    if (isYellowLedOn && millis() > yellowLedOffTime) {
        digitalWrite(YELLOW_LED_PIN, LOW);
        isYellowLedOn = false;
    }

    if (isBlueLedOn && millis() > blueLedOffTime) {
        digitalWrite(BLUE_LED_PIN, LOW);
        isBlueLedOn = false;
    }
}