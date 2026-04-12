/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <Arduino.h>
#include <RadioLib.h>
#include <ArduinoJson.h>
#include "common/LoraMailBox_NTFY_types.h"
#include "user_settings/user_settings.h"

SX1262 radio = nullptr;

/** @brief Save transmission state between loops. */
int transmissionState = RADIOLIB_ERR_NONE;

/** @brief Flag indicating that a transmission is in progress. */
bool transmitFlag = false;

/** @brief Flag set by the radio ISR when a packet was sent or received. */
volatile bool loraEvent = false;

enum class TxTrigger : uint8_t
{
    Boot,
    WakeupPinHigh,
    HeartbeatTx,
};

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

static inline const char *txTriggerToString(TxTrigger trigger)
{
    switch (trigger)
    {
    case TxTrigger::Boot:
        return "BOOT";
    case TxTrigger::WakeupPinHigh:
        return "WAKEUP_PIN_HIGH";
    case TxTrigger::HeartbeatTx:
        return "HEARTBEAT_TX";
    default:
        return "BOOT";
    }
}

/**
 * @brief RadioLib IRQ callback for TX/RX completion.
 * @note This function must have `void` return type and no arguments.
 */
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setFlag(void)
{
    loraEvent = true;
}

void debounce(uint32_t wait)
{
    delay(wait - millis() % 1000);
}

void blink(
    unsigned long on_duration_ms,
    unsigned long total_duration_ms,
    unsigned long repeat,
    uint32_t led_pin,
    bool invert)
{
    pinMode(led_pin, OUTPUT);
    for (size_t i = 0; i < repeat; i++)
    {
        digitalWrite(led_pin, invert ? LOW : HIGH);
        delay(on_duration_ms);
        digitalWrite(led_pin, invert ? HIGH : LOW);
        delay(total_duration_ms - on_duration_ms);
    }
}

void sendLoRaPayload(const String &payload)
{
    Serial.printf("%sSending\t\t%s", PREFIX, payload.c_str());

    /**
     * @note Do not use the non-blocking `startTransmit()` function here.
     * @note It makes it difficult to estimate the required delay before sending a new message.
     */
    int state = radio.transmit(payload.c_str());
    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf("%sFailed, code %d", PREFIX, state);
        return;
    }
    Serial.print(PREFIX);
    Serial.print(F("Transmission finished!"));
}

void setupLoRa()
{
    radio = new Module(settings::board::lora_cs,
                       settings::board::lora_irq,
                       settings::board::lora_rst,
                       settings::board::lora_gpio);
    Serial.print(PREFIX);
    Serial.print(F("Initializing LoRa..."));
    int state = radio.begin(
        settings::lora::freq,
        settings::lora::bw,
        settings::lora::sf,
        settings::lora::cr,
        settings::lora::syncword,
        settings::lora::power,
        settings::lora::preamble_length,
        settings::lora::tcxo_voltage,
        settings::lora::use_regulator_ldo);
    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf(" failed, code %d\n", state);
        while (true)
            yield();
    }
    Serial.print(F(" success"));
}

void printSplashScreen()
{
    Serial.println("\n\n##########################");
    Serial.print(F("PROJECT NAME:     "));
    Serial.println(PROJECT_NAME);
    Serial.print(F("FILE NAME:        "));
    Serial.println(__FILE__);
    Serial.print(F("PROJECT PATH:     "));
    Serial.println(PROJECT_PATH);
    Serial.print(F("COMPILATION DATE: "));
    Serial.println(COMPILATION_DATE);
    Serial.print(F("COMPILATION TIME: "));
    Serial.println(COMPILATION_TIME);
    Serial.print(F("LAST COMMIT ID:   "));
    Serial.println(LAST_COMMIT_ID);
    Serial.print(F("PYTHON VERSION:   "));
    Serial.println(PYTHON_VERSION);
    Serial.print(F("PYTHON PATH:      "));
    Serial.println(PYTHON_PATH);
    Serial.print(F("F_CPU:            "));
    Serial.println(F_CPU);
    Serial.print(F("MCU Model:        "));
#if defined(ESP32)
    Serial.println(ESP.getChipModel());
#elif defined(NRF52_SERIES)
    Serial.print(F("nRF"));
    Serial.println(NRF_FICR->INFO.PART, HEX);
#endif
    Serial.println("##########################\n\n");
}

void setupSerial()
{
    Serial.begin(BAUD_RATE);
}
