/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <Arduino.h>
#include <RadioLib.h>
#include <ArduinoJson.h>
#include "common/tx_types.h"
#include "common/ntfy_types.h"
#include "common/battery.h"
#include "user_settings/user_settings.h"
#include "common/blinker.h"

extern Blinker statusLed;


extern SX1262 radio;

/** @brief Save transmission state between loops. */
extern int transmissionState;

/** @brief Flag indicating that a transmission is in progress. */
extern bool transmitFlag;

/** @brief Flag set by the radio ISR when a packet was sent or received. */
extern volatile bool loraEvent;

static constexpr char PREFIX[] = "\n[" PROJECT_NAME "] ";

/**
 * @brief RadioLib IRQ callback for TX/RX completion.
 * @note This function must have `void` return type and no arguments.
 */
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setFlag(void);

void debounce(uint32_t wait);

void blink(
    uint32_t on_duration_ms,
    uint32_t total_duration_ms,
    uint32_t repeat,
    uint32_t led_pin,
    bool invert);

void sendLoRaPayload(const char *payload);

void setupLoRa();

void printSplashScreen();

void setupSerial();
