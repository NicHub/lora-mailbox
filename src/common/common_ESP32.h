/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <Arduino.h>
#include <RadioLib.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "user_settings/user_settings.h"

#define MASK (1ULL << board::hw::wakeup_pin)

#define PREFIX "\n[" PROJECT_NAME "] "

void debounce(uint32_t);
void blink(unsigned long, unsigned long, unsigned long, uint32_t, bool);

// Platform counter storage API.
// ESP32 RX does not currently persist a message counter, but we keep the
// same interface as nRF52 so a future ESP-based TX can reuse the same calls.
void setupMsgCounterStorage()
{
}

uint16_t readMsgCounter()
{
    return 0;
}

void saveMsgCounter(uint16_t cnt)
{
    (void)cnt;
}

void setupDeepSleep()
{
    esp_sleep_enable_ext1_wakeup(MASK, ESP_EXT1_WAKEUP_ANY_HIGH);
}

void goToDeepSleep()
{
    Serial.printf(PREFIX "COMPILATION DATE %s", COMPILATION_DATE);
    Serial.printf(PREFIX "COMPILATION TIME %s", COMPILATION_TIME);
    Serial.printf(PREFIX "PROJECT NAME %s", PROJECT_NAME);
    Serial.print(F(PREFIX "GOING TO DEEP SLEEP\n"));
    Serial.flush();

    setupDeepSleep();
    debounce(1000);
    digitalWrite(board::hw::lora_led_green, LOW);
    esp_deep_sleep_start();
}

void switchOffAllLEDs()
{
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(board::hw::lora_led_green, LOW);
}

uint16_t readBatteryVoltage()
{
    // TODO
    return 0;
}

/**
 * @brief Return ESP32 station MAC address as `XX:XX:XX:XX:XX:XX`.
 * @return MAC address string.
 */
static inline String getMacAddress()
{
    return WiFi.macAddress();
}
