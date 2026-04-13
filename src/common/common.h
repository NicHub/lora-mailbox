/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <Arduino.h>
#include <RadioLib.h>
#include <ArduinoJson.h>
#include "common/LoraMailBox_TX_types.h"
#include "common/LoraMailBox_NTFY_types.h"
#include "user_settings/user_settings.h"
#include "common/Blinker.h"

extern Blinker statusLed;


extern SX1262 radio;

/** @brief Save transmission state between loops. */
extern int transmissionState;

/** @brief Flag indicating that a transmission is in progress. */
extern bool transmitFlag;

/** @brief Flag set by the radio ISR when a packet was sent or received. */
extern volatile bool loraEvent;

static constexpr char PREFIX[] = "\n[" PROJECT_NAME "] ";


enum class BatteryStatus : uint8_t
{
    High,
    Ok,
    Low,
    NoBattery,
};

enum class BatteryGlyph : uint8_t
{
    Charging,
    ExternalPower,
    Empty,
    Quarter,
    Half,
    ThreeQuarters,
    Full,
};

struct BatteryMeasurement
{
    int vbatMv;
    int batteryPercent;
    BatteryGlyph glyph;
    BatteryStatus status;
};

static inline BatteryMeasurement Vbat_raw2Vbat_mv(uint16_t vgpioMv)
{
    BatteryMeasurement measurement{};

    /** @note Convert the raw GPIO-derived voltage to an estimated battery voltage. */
    measurement.vbatMv = static_cast<int>(
        settings::battery::fit_slope * static_cast<float>(vgpioMv) + settings::battery::fit_offset);

    /** @note Map the estimated battery voltage to a 0..100 percent range. */
    if (measurement.vbatMv >= settings::battery::max)
        measurement.batteryPercent = 100;
    else if (measurement.vbatMv <= settings::battery::min)
        measurement.batteryPercent = 0;
    else
        measurement.batteryPercent = static_cast<int>(
            (100.f * static_cast<float>(measurement.vbatMv - settings::battery::min)) /
                static_cast<float>(settings::battery::max - settings::battery::min) +
            0.5f);

    /** @note Pick a compact battery glyph for quick display in notifications. */
    if (measurement.vbatMv >= settings::battery::max)
        measurement.glyph = BatteryGlyph::Charging;
    else if (measurement.vbatMv < settings::battery::no_battery_threshold)
        measurement.glyph = BatteryGlyph::ExternalPower;
    else if (measurement.batteryPercent < 5)
        measurement.glyph = BatteryGlyph::Empty;
    else if (measurement.batteryPercent < 25)
        measurement.glyph = BatteryGlyph::Quarter;
    else if (measurement.batteryPercent < 50)
        measurement.glyph = BatteryGlyph::Half;
    else if (measurement.batteryPercent < 75)
        measurement.glyph = BatteryGlyph::ThreeQuarters;
    else
        measurement.glyph = BatteryGlyph::Full;

    /** @note Derive the coarse battery status used by MQTT/NTFY routing logic. */
    if (measurement.vbatMv >= settings::battery::max)
        measurement.status = BatteryStatus::High;
    else if (measurement.vbatMv >= settings::battery::min)
        measurement.status = BatteryStatus::Ok;
    else if (measurement.vbatMv >= settings::battery::no_battery_threshold)
        measurement.status = BatteryStatus::Low;
    else
        measurement.status = BatteryStatus::NoBattery;

    return measurement;
}

static inline const char *batteryGlyphToString(BatteryGlyph glyph)
{
    switch (glyph)
    {
    case BatteryGlyph::Charging:
        return "⚡";
    case BatteryGlyph::ExternalPower:
        return "🔌";
    case BatteryGlyph::Empty:
        return "▁";
    case BatteryGlyph::Quarter:
        return "▂";
    case BatteryGlyph::Half:
        return "▄";
    case BatteryGlyph::ThreeQuarters:
        return "▆";
    case BatteryGlyph::Full:
        return "█";
    default:
        return "";
    }
}

static inline const char *batteryStatusToString(BatteryStatus status)
{
    switch (status)
    {
    case BatteryStatus::High:
        return "HIGH";
    case BatteryStatus::Ok:
        return "OK";
    case BatteryStatus::Low:
        return "LOW";
    case BatteryStatus::NoBattery:
        return "NOBAT";
    default:
        return "NOBAT";
    }
}

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

