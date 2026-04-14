/**
 * LoRa MailBox — Battery
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <Arduino.h>
#include "user_settings/user_settings.h"

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
    int vbat_mv;
    int battery_percent;
    BatteryGlyph glyph;
    BatteryStatus status;
};

static inline BatteryMeasurement vbatRaw2VbatMv(uint16_t vgpio_mv)
{
    BatteryMeasurement measurement{};

    /** @note Convert the raw GPIO-derived voltage to an estimated battery voltage. */
    measurement.vbat_mv = static_cast<int>(
        settings::battery::FIT_SLOPE * static_cast<float>(vgpio_mv) + settings::battery::FIT_OFFSET);

    /** @note Map the estimated battery voltage to a 0..100 percent range. */
    if (measurement.vbat_mv >= settings::battery::MAX)
        measurement.battery_percent = 100;
    else if (measurement.vbat_mv <= settings::battery::MIN)
        measurement.battery_percent = 0;
    else
        measurement.battery_percent = static_cast<int>(
            (100.f * static_cast<float>(measurement.vbat_mv - settings::battery::MIN)) /
                static_cast<float>(settings::battery::MAX - settings::battery::MIN) +
            0.5f);

    /** @note Pick a compact battery glyph for quick display in notifications. */
    if (measurement.vbat_mv >= settings::battery::MAX)
        measurement.glyph = BatteryGlyph::Charging;
    else if (measurement.vbat_mv < settings::battery::NO_BATTERY_THRESHOLD)
        measurement.glyph = BatteryGlyph::ExternalPower;
    else if (measurement.battery_percent < 5)
        measurement.glyph = BatteryGlyph::Empty;
    else if (measurement.battery_percent < 25)
        measurement.glyph = BatteryGlyph::Quarter;
    else if (measurement.battery_percent < 50)
        measurement.glyph = BatteryGlyph::Half;
    else if (measurement.battery_percent < 75)
        measurement.glyph = BatteryGlyph::ThreeQuarters;
    else
        measurement.glyph = BatteryGlyph::Full;

    /** @note Derive the coarse battery status used by MQTT/NTFY routing logic. */
    if (measurement.vbat_mv >= settings::battery::MAX)
        measurement.status = BatteryStatus::High;
    else if (measurement.vbat_mv >= settings::battery::MIN)
        measurement.status = BatteryStatus::Ok;
    else if (measurement.vbat_mv >= settings::battery::NO_BATTERY_THRESHOLD)
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
