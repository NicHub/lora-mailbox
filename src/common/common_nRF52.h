/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <Arduino.h>
#include <RadioLib.h>
#include <ArduinoJson.h>
#include "flash/flash_nrf5x.h"

static constexpr char PREFIX[] = "\n[" PROJECT_NAME "] ";

#include "common/common.h"

void debounce(uint32_t);
void blink(unsigned long, unsigned long, unsigned long, uint32_t, bool);

#ifdef NRF52840_XXAA
static constexpr uint32_t COUNTER_STORAGE_ADDR = 0xED000;
#else
static constexpr uint32_t COUNTER_STORAGE_ADDR = 0x6D000;
#endif

static constexpr uint32_t COUNTER_STORAGE_SIZE = 7 * FLASH_NRF52_PAGE_SIZE;

struct CounterRecord
{
    uint16_t counter;
    uint16_t checksum;
    uint32_t magic;
};

static_assert(sizeof(CounterRecord) == 8, "CounterRecord must stay 8 bytes");

static constexpr uint32_t COUNTER_RECORD_MAGIC = 0x4D424358UL;
static constexpr uint16_t COUNTER_RECORD_XOR = 0xA5C3U;
static int32_t counterRecordIndexCache = -2;

static inline uint16_t counterChecksum(uint16_t counter)
{
    return static_cast<uint16_t>(counter ^ COUNTER_RECORD_XOR);
}

static inline bool isCounterRecordValid(const CounterRecord &record)
{
    return record.magic == COUNTER_RECORD_MAGIC && record.checksum == counterChecksum(record.counter);
}

static inline uint32_t counterRecordCapacity()
{
    return COUNTER_STORAGE_SIZE / sizeof(CounterRecord);
}

static inline uint32_t counterRecordAddress(uint32_t index)
{
    return COUNTER_STORAGE_ADDR + index * sizeof(CounterRecord);
}

static inline CounterRecord readCounterRecord(uint32_t index)
{
    CounterRecord record{};
    flash_nrf5x_read(&record, counterRecordAddress(index), sizeof(record));
    return record;
}

static inline bool eraseCounterStorage()
{
    for (uint32_t addr = COUNTER_STORAGE_ADDR; addr < COUNTER_STORAGE_ADDR + COUNTER_STORAGE_SIZE; addr += FLASH_NRF52_PAGE_SIZE)
    {
        if (!flash_nrf5x_erase(addr))
            return false;
    }
    flash_nrf5x_flush();
    return true;
}

static inline int32_t findLastCounterRecordIndex()
{
    int32_t lastValidIndex = -1;
    for (uint32_t index = 0; index < counterRecordCapacity(); ++index)
    {
        CounterRecord record = readCounterRecord(index);
        if (record.magic == 0xFFFFFFFFUL)
            break;
        if (!isCounterRecordValid(record))
            break;
        lastValidIndex = static_cast<int32_t>(index);
    }
    return lastValidIndex;
}

static inline int32_t getLastCounterRecordIndex()
{
    if (counterRecordIndexCache == -2)
        counterRecordIndexCache = findLastCounterRecordIndex();
    return counterRecordIndexCache;
}

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
    pinMode(settings::board::wakeup_pin, INPUT_SENSE_HIGH);
    debounce(1000);
    NRF_POWER->SYSTEMOFF = 1;
}

uint16_t readMsgCounter()
{
    int32_t lastRecordIndex = getLastCounterRecordIndex();
    if (lastRecordIndex < 0)
        return 0;
    return readCounterRecord(static_cast<uint32_t>(lastRecordIndex)).counter;
}

void saveMsgCounter(uint16_t cnt)
{
    uint32_t t1 = millis();
    int32_t lastRecordIndex = getLastCounterRecordIndex();
    uint32_t nextRecordIndex = static_cast<uint32_t>(lastRecordIndex + 1);
    if (nextRecordIndex >= counterRecordCapacity())
    {
        if (!eraseCounterStorage())
        {
            Serial.println("Counter storage erase failed");
            return;
        }
        nextRecordIndex = 0;
        counterRecordIndexCache = -1;
    }

    CounterRecord record{
        .counter = cnt,
        .checksum = counterChecksum(cnt),
        .magic = COUNTER_RECORD_MAGIC,
    };
    if (flash_nrf5x_write(counterRecordAddress(nextRecordIndex), &record, sizeof(record)) < 0)
    {
        Serial.println("Counter storage write failed");
        return;
    }
    flash_nrf5x_flush();
    counterRecordIndexCache = static_cast<int32_t>(nextRecordIndex);

    Serial.print("\n\nmillis(), Time elapsed to saveMsgCounter = ");
    Serial.print(millis());
    Serial.print("\t");
    Serial.print(millis() - t1);
}

void setupMsgCounterStorage()
{
    counterRecordIndexCache = findLastCounterRecordIndex();
    int32_t lastRecordIndex = counterRecordIndexCache;
    if (lastRecordIndex >= 0)
        return;

    if (!eraseCounterStorage())
    {
        Serial.println("Counter storage init failed");
        blink(10, 100, 10, LED_RED, true);
        return;
    }
    saveMsgCounter(0);
    if (readMsgCounter() != 0)
    {
        Serial.println("Counter storage verification failed");
        blink(10, 100, 10, LED_RED, true);
    }
}

uint16_t readBatteryVoltage()
{
    /*
        VBAT_ENABLE controls the VBAT voltage divider for PIN_VBAT.
        See: https://wiki.seeedstudio.com/XIAO_BLE/#q3-what-are-the-considerations-when-using-xiao-nrf52840-sense-for-battery-charging

        pinMode   digitalWrite   SAFE   USAGE             EFFECT
        INPUT     LOW            YES    low consumption   Divider off (high-Z)
        INPUT     HIGH           YES    low consumption   Divider off (high-Z)
        OUTPUT    LOW            YES    bat read          Divider on → VBAT readable on PIN_VBAT
        OUTPUT    HIGH           NO     avoid             Forces 3.3V → may exceed 3.6V on PIN_VBAT

        Recommended sequence:
        pinMode(VBAT_ENABLE, OUTPUT);
        digitalWrite(VBAT_ENABLE, LOW);   // enable divider
        int vbat = analogRead(A0);
        pinMode(VBAT_ENABLE, INPUT);      // disable divider
    */

    pinMode(VBAT_ENABLE, OUTPUT);
    digitalWrite(VBAT_ENABLE, LOW);
    pinMode(PIN_VBAT, INPUT);

    // Let the analog node settle (RC + ADC input)
    delay(2); // ms (paranoid but cheap)
    // Discard first sample (often "dirty" after switching)
    (void)analogRead(PIN_VBAT);

    uint32_t vbat = 0;
    uint32_t sum = 0;
    constexpr int CNT_MAX = 10;
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

    constexpr uint32_t LED_ON = LOW;
    constexpr uint32_t LED_OFF = HIGH;

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

static inline void writeRgbLeds(
    uint32_t LED_RED_STATE,
    uint32_t LED_GREEN_STATE,
    uint32_t LED_BLUE_STATE)
{
    if (settings::misc::debug)
    {
        // Note that on XIAO nRF52, LED_BUILTIN == LED_RED.
        digitalWrite(LED_RED, !LED_RED_STATE);
        digitalWrite(LED_GREEN, !LED_GREEN_STATE);
        digitalWrite(LED_BLUE, !LED_BLUE_STATE);
    }
    else
    {
        (void)LED_RED_STATE;
        (void)LED_GREEN_STATE;
        (void)LED_BLUE_STATE;
    }
}
