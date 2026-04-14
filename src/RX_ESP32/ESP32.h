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

static constexpr uint64_t MASK = 1ULL << settings::board::WAKEUP_PIN;

void debounce(uint32_t);
void blink(uint32_t, uint32_t, uint32_t, uint32_t, bool);

/**
 * @note Platform counter storage API.
 * @note ESP32 RX does not currently persist a message counter, but it keeps the
 * same interface as nRF52 so a future ESP-based TX can reuse the same calls.
 */
void setupMsgCounterStorage();

uint16_t readMsgCounter();

void saveMsgCounter(uint16_t cnt);

void setupDeepSleep();

void goToDeepSleep();

void switchOffAllLEDs();

uint16_t readBatteryVoltage();

/**
 * @brief Return ESP32 station MAC address as `XX:XX:XX:XX:XX:XX`.
 * @return MAC address string.
 */
static inline String getMacAddress()
{
    return WiFi.macAddress();
}
