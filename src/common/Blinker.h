/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <Arduino.h>

class Blinker {
    bool active = false;
    uint32_t onTime = 0;
    uint32_t totalTime = 0;
    uint32_t blinkCount = 0;
    uint32_t currentCount = 0;
    uint32_t pin = 0;
    bool invert = false;
    uint32_t lastChangeTime = 0;
    bool isOn = false;

public:
    void start(uint32_t on_ms, uint32_t total_ms, uint32_t count, uint32_t led_pin, bool inv = false) {
        onTime = on_ms;
        totalTime = total_ms;
        blinkCount = count;
        pin = led_pin;
        invert = inv;
        currentCount = 0;
        active = true;
        isOn = true;
        lastChangeTime = millis();
        pinMode(pin, OUTPUT);
        digitalWrite(pin, invert ? LOW : HIGH);
    }

    void update() {
        if (!active) return;
        uint32_t now = millis();
        uint32_t elapsed = now - lastChangeTime;

        if (isOn) {
            if (elapsed >= onTime) {
                isOn = false;
                lastChangeTime = now;
                digitalWrite(pin, invert ? HIGH : LOW);
            }
        } else {
            if (elapsed >= (totalTime - onTime)) {
                currentCount++;
                if (currentCount >= blinkCount) {
                    active = false;
                    digitalWrite(pin, invert ? HIGH : LOW); // Ensure off
                } else {
                    isOn = true;
                    lastChangeTime = now;
                    digitalWrite(pin, invert ? LOW : HIGH);
                }
            }
        }
    }
    
    bool isBlinking() const { return active; }
};
