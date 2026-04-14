/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <Arduino.h>

class Blinker
{
    bool active = false;
    uint32_t on_time = 0;
    uint32_t total_time = 0;
    uint32_t blink_count = 0;
    uint32_t current_count = 0;
    uint32_t pin = 0;
    bool invert = false;
    uint32_t last_change_time = 0;
    bool is_on = false;

  public:
    void
    start(uint32_t on_ms, uint32_t total_ms, uint32_t count, uint32_t led_pin, bool inv = false)
    {
        on_time = on_ms;
        total_time = total_ms;
        blink_count = count;
        pin = led_pin;
        invert = inv;
        current_count = 0;
        active = true;
        is_on = true;
        last_change_time = millis();
        pinMode(pin, OUTPUT);
        digitalWrite(pin, invert ? LOW : HIGH);
    }

    void update()
    {
        if (!active)
            return;
        uint32_t now = millis();
        uint32_t elapsed = now - last_change_time;

        if (is_on)
        {
            if (elapsed >= on_time)
            {
                is_on = false;
                last_change_time = now;
                digitalWrite(pin, invert ? HIGH : LOW);
            }
        }
        else
        {
            if (elapsed >= (total_time - on_time))
            {
                current_count++;
                if (current_count >= blink_count)
                {
                    active = false;
                    digitalWrite(pin, invert ? HIGH : LOW); // Ensure off
                }
                else
                {
                    is_on = true;
                    last_change_time = now;
                    digitalWrite(pin, invert ? LOW : HIGH);
                }
            }
        }
    }

    bool isBlinking() const
    {
        return active;
    }
};
