/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <Arduino.h>
#include <RadioLib.h>
#include <ArduinoJson.h>
#include "common/LoraMailBox_Settings.h"

// #if defined(ESP32)
// #include "common/common_ESP32.h"
// #elif defined(NRF52840_XXAA)
// #include "common/common_nRF52.h"
// #endif

SX1262 radio = nullptr;

// Save transmission states between loops.
int transmissionState = RADIOLIB_ERR_NONE;

// Flag to indicate transmission or reception state.
bool transmitFlag = false;

// Flag to indicate that a packet was sent or received.
volatile bool loraEvent = false;

// The `setFlag` function is called when a complete
// packet is transmitted or received by the module
// IMPORTANT: this function MUST be 'void' type and
// MUST NOT have any arguments!
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
    unsigned long on_duration_ms = 10,
    unsigned long total_duration_ms = 100,
    unsigned long repeat = 10,
    uint32_t led_pin = LORA_LED_GREEN,
    bool invert = false)
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

void transmitLoRa(uint8_t board_id, uint16_t cnt, uint16_t battery_voltage)
{
    String msg;
    JsonDocument doc;
    doc["cnt"] = cnt;
    doc["board id"] = board_id;
    doc["volt"] = battery_voltage;
    serializeJson(doc, msg);

    Serial.printf(PREFIX "Sending\t\t%s", msg.c_str());

    // Don’t use the non-blocking `startTransmit()` function.
    // It makes it difficult to estimate the necessary delay
    // before sending a new message.
    int state = radio.transmit(msg.c_str());
    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf(PREFIX "Failed, code %d", state);
        return;
    }
    Serial.print(F(PREFIX "Transmission finished!"));
}

void setupLoRa()
{
    radio = new Module(CS, IRQ, RST, LORA_GPIO_PIN);
    Serial.print(F(PREFIX "Initializing LoRa..."));
    int state = radio.begin(
        FREQ,
        BW,
        SF,
        CR,
        SYNCWORD,
        POWER,
        PREAMBLELENGTH,
        TCXOVOLTAGE,
        USEREGULATORLDO);
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
    Serial.print(F("# PROJECT PATH:     "));
    Serial.println(PROJECT_PATH);
    Serial.print(F("# PROJECT NAME:     "));
    Serial.println(PROJECT_NAME);
    Serial.print(F("# COMPILATION DATE: "));
    Serial.println(COMPILATION_DATE);
    Serial.print(F("# COMPILATION TIME: "));
    Serial.println(COMPILATION_TIME);
    Serial.print(F("# F_CPU:            "));
    Serial.println(F_CPU);
    Serial.print(F("# LAST_COMMIT_ID:   "));
    Serial.println(LAST_COMMIT_ID);
    Serial.println("##########################\n\n");
}

void setupSerial(size_t printCnt = 0)
{
    Serial.begin(BAUD_RATE);
    for (size_t i = 0; i <= printCnt; i++)
    {
        Serial.println(i);
        delay(1000);
    }
}
