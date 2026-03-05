/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <Arduino.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <RadioLib.h>
#include <ArduinoJson.h>

// https://github.com/meshtastic/firmware/blob/master/variants/seeed_xiao_nrf52840_kit/variant.h
// Wio-SX1262 for XIAO (standalone SKU 113010003 or nRF52840 kit SKU 102010710)

// LoRa module SX1262 for nRF52
// https://files.seeedstudio.com/products/SenseCAP/Wio_SX1262/Wio-SX1262%20for%20XIAO%20V1.0_SCH.pdf

// LoRa module SX1262 for ESP32
// https://files.seeedstudio.com/products/SenseCAP/Wio_SX1262/Schematic_Diagram_Wio-SX1262_for_XIAO.pdf

#define CS D4
#define IRQ D1
#define RST D2
#define LORA_GPIO_PIN D3
#define LORA_LED_GREEN LED_GREEN

using namespace Adafruit_LittleFS_Namespace;

#define NO_HEARTBEAT_PIN D0
#define LORA_USER_BUTTON GPIO_NUM_21

#define CNT_LOG_FILENAME "/cnt.log"

#define PREFIX "\n[" PROJECT_NAME "] "

File file(InternalFS);
Adafruit_LittleFS littleFS;

void debounce(uint32_t);

/*
 * 64 bit-long BOARD ID
 *   => 20 digit-long in DEC
 */
static inline uint64_t getBoardUidDec()
{
    static const uint64_t uid =
        ((uint64_t)NRF_FICR->DEVICEID[1] << 32) | NRF_FICR->DEVICEID[0];
    return uid;
}

/*
 * 64 bit-long BOARD ID
 *   => 16 digit-long in HEX
 */
static inline String getBoardUidHex()
{
    static char uid_hex[17];
    static bool initialized = false;

    if (initialized)
        return String(uid_hex);

    // Avoid %llX on embedded printf/nano-libc:
    // format the two 32-bit words directly.
    uint32_t uid_hi = NRF_FICR->DEVICEID[1];
    uint32_t uid_lo = NRF_FICR->DEVICEID[0];
    snprintf(
        uid_hex,
        sizeof(uid_hex),
        "%08lX%08lX",
        (unsigned long)uid_hi,
        (unsigned long)uid_lo);
    initialized = true;

    return String(uid_hex);
}

/*
 * Deep sleep from
 * https://forum.seeedstudio.com/t/xiao-sense-accelerometer-examples-and-low-power/270801
 */
void goToDeepSleep()
{
    pinMode(WAKEUP_PIN, INPUT_SENSE_HIGH);
    debounce(1000);
    NRF_POWER->SYSTEMOFF = 1;
}

uint16_t readMsgCounterFromFile()
{
    file = InternalFS.open(CNT_LOG_FILENAME, FILE_O_READ);
    uint16_t cnt = file.readString().toInt();
    file.close();
    return cnt;
}

void saveMsgCounterToFile(uint16_t cnt)
{
    // This function is awkwardly slow, it takes 383 ms to save a number to
    // a file. So it must be executed after the message is sent via LoRa.

    // For whatever reason, the built in LED is switched on by the functions below.
    // So we’ll bring it back to its initial state.
    uint32_t t1 = millis();
    bool initialLEDstate = digitalRead(LED_BUILTIN);
    if (InternalFS.exists(CNT_LOG_FILENAME))
        InternalFS.remove(CNT_LOG_FILENAME);
    digitalWrite(LED_BUILTIN, initialLEDstate);
    file = InternalFS.open(CNT_LOG_FILENAME, FILE_O_WRITE);
    digitalWrite(LED_BUILTIN, initialLEDstate);
    file.print(cnt, 10);
    digitalWrite(LED_BUILTIN, initialLEDstate);
    file.close();
    digitalWrite(LED_BUILTIN, initialLEDstate);
    Serial.print("\n\nmillis(), Time elapsed to saveMsgCounterToFile = ");
    Serial.print(millis());
    Serial.print("\t");
    Serial.print(millis() - t1);
}

void blink(unsigned long, unsigned long, unsigned long, uint32_t, bool);

void setupLittleFS()
{
    bool state = InternalFS.begin();
    while (!state)
    {
        Serial.println("InternalFS.begin() failed");
        blink(10, 100, 10, LED_RED, true);
    }

#if defined(FORMAT_LITTLEFS) && FORMAT_LITTLEFS == 1
    digitalWrite(LED_GREEN, LOW);
    InternalFS.format();
    digitalWrite(LED_GREEN, HIGH);
    while (true)
    {
        Serial.println("Format LittleFS done.");
        Serial.println("Set FORMAT_LITTLEFS to 0 and reflash the board.\n");
        blink(10, 100, 10, LED_BLUE, true);
    }
#endif

    if (!InternalFS.exists(CNT_LOG_FILENAME))
        saveMsgCounterToFile(0);
}

uint16_t readBatteryVoltage()
{
    // Set VBAT_ENABLE to OUTPUT LOW to read VBAT.
    // Never set it HIGH during charging (risk of damaging PIN_VBAT).
    // See: https://wiki.seeedstudio.com/XIAO_BLE/#q3-what-are-the-considerations-when-using-xiao-nrf52840-sense-for-battery-charging
    pinMode(VBAT_ENABLE, OUTPUT);
    digitalWrite(VBAT_ENABLE, LOW);
    pinMode(PIN_VBAT, INPUT);

    // Let the analog node settle (RC + ADC input)
    delay(2); // ms (paranoid but cheap)
    // Discard first sample (often "dirty" after switching)
    (void)analogRead(PIN_VBAT);

    uint32_t vbat = 0;
    uint32_t sum = 0;
#define CNT_MAX 10
    for (int i = 0; i < CNT_MAX; ++i)
    {
        sum += analogRead(PIN_VBAT);
        delayMicroseconds(200);
    }
    vbat = sum / CNT_MAX;

    // Reset to INPUT after reading to avoid current draw through the resistor divider.
    pinMode(VBAT_ENABLE, INPUT);
    return vbat;
}

void testAllLEDs()
{
    Serial.println("\n================");
    uint32_t wait_1 = 5000;
    uint32_t wait_2 = 1000;

#define LED_ON LOW
#define LED_OFF HIGH

    digitalWrite(LED_BUILTIN, LED_ON);
    Serial.println("\nLED_BUILTIN, LED_ON");
    delay(wait_1);
    digitalWrite(LED_BUILTIN, LED_OFF);
    Serial.println("LED_BUILTIN, LED_OFF");
    delay(wait_2);

    digitalWrite(LED_RED, LED_ON);
    Serial.println("\nLED_RED, LED_ON");
    delay(wait_1);
    digitalWrite(LED_RED, LED_OFF);
    Serial.println("LED_RED, LED_OFF");
    delay(wait_2);

    digitalWrite(LED_GREEN, LED_ON);
    Serial.println("\nLED_GREEN, LED_ON");
    delay(wait_1);
    digitalWrite(LED_GREEN, LED_OFF);
    Serial.println("LED_GREEN, LED_OFF");
    delay(wait_2);

    digitalWrite(LED_BLUE, LED_ON);
    Serial.println("\nLED_BLUE, LED_ON");
    delay(wait_1);
    digitalWrite(LED_BLUE, LED_OFF);
    Serial.println("LED_BLUE, LED_OFF");
    delay(wait_2);
}

#if DEBUG
static inline void writeRgbLeds(
    uint32_t LED_RED_STATE,
    uint32_t LED_GREEN_STATE,
    uint32_t LED_BLUE_STATE)
{
    // Note that on XIAO nRF52, LED_BUILTIN == LED_RED.
    digitalWrite(LED_RED, !LED_RED_STATE);
    digitalWrite(LED_GREEN, !LED_GREEN_STATE);
    digitalWrite(LED_BLUE, !LED_BLUE_STATE);
}
#else
static inline void writeRgbLeds(uint32_t, uint32_t, uint32_t) {}
#endif
