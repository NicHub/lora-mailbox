/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include "common/common.h"

Module lora_module(
    settings::board::LORA_CS,
    settings::board::LORA_IRQ,
    settings::board::LORA_RST,
    settings::board::LORA_GPIO);
SX1262 radio(&lora_module);

Blinker statusLed;

/**
 * @note For now, the LoRa profile index can only be chosen at startup.
 * @note A future version must allow changing this index at runtime and
 * reconfiguring the radio without rebooting.
 */
size_t settings::lora::detail::current_lora_profile_index = settings::lora::DEFAULT_LORA_PROFILE_INDEX;

/** @brief Save transmission state between loops. */
int transmissionState = RADIOLIB_ERR_NONE;

/** @brief Flag indicating that a transmission is in progress. */
bool transmitFlag = false;

/** @brief Flag set by the radio ISR when a packet was sent or received. */
volatile bool loraEvent = false;

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
    uint32_t on_duration_ms,
    uint32_t total_duration_ms,
    uint32_t repeat,
    uint32_t led_pin,
    bool invert)
{
    statusLed.start(on_duration_ms, total_duration_ms, repeat, led_pin, invert);
    while (statusLed.isBlinking())
    {
        statusLed.update();
        delay(1);
    }
}

void sendLoRaPayload(const char *payload)
{
    Serial.printf("%sSending\t\t%s", PREFIX, payload);

    /**
     * @note Do not use the non-blocking `startTransmit()` function here.
     * @note It makes it difficult to estimate the required delay before sending a new message.
     */
    int state = radio.transmit(payload);
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
    Serial.print(PREFIX);
    Serial.print(F("Initializing LoRa..."));
    const settings::lora::Parameters &lora = settings::lora::current();
    int state = radio.begin(
        lora.freq,
        lora.bw,
        lora.sf,
        lora.cr,
        lora.syncword,
        lora.power,
        lora.preamble_length,
        lora.tcxo_voltage,
        lora.use_regulator_ldo);
    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf(" failed, code %d\n", state);
        while (true)
            yield();
    }
    Serial.print(F(" success"));
    Serial.print(F(" profile="));
    Serial.print(lora.name);
}

void printSplashScreen()
{
    Serial.println("\n\n##########################");
    Serial.print(F("PROJECT NAME:       "));
    Serial.println(PROJECT_NAME);
    Serial.print(F("FILE NAME:          "));
    Serial.println(__FILE__);
    Serial.print(F("BUILD_SOURCE_PATH:  "));
    Serial.println(BUILD_SOURCE_PATH);
    Serial.print(F("BUILD_LOCAL_TIME:   "));
    Serial.println(BUILD_LOCAL_TIME);
    Serial.print(F("GIT_HEAD_COMMIT_ID: "));
    Serial.println(GIT_HEAD_COMMIT_ID);
    Serial.print(F("GIT_UNCOMMITTED:    "));
    Serial.println(GIT_UNCOMMITTED_FILES_COUNT);
    Serial.print(F("BUILD_PYTHON_VER:   "));
    Serial.println(BUILD_PYTHON_VERSION);
    Serial.print(F("BUILD_PYTHON_PATH:  "));
    Serial.println(BUILD_PYTHON_PATH);
    Serial.print(F("F_CPU:              "));
    Serial.println(F_CPU);
    Serial.print(F("MCU_MODEL:          "));
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
