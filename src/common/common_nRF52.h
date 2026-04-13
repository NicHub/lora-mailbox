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



#include "common/common.h"

void debounce(uint32_t);
void blink(uint32_t, uint32_t, uint32_t, uint32_t, bool);

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

static inline String buildTxPayload(
    const char* board_id,
    uint16_t cnt,
    uint16_t vbat_raw,
    TxTrigger tx_trigger)
{
    JsonDocument doc;
    doc["TX_BOARD_ID"] = board_id;
    doc["TX_COUNTER"] = cnt;
    doc["TX_DEBUG"] = settings::misc::debug;
    doc["TX_GIT_HEAD_COMMIT_ID"] = GIT_HEAD_COMMIT_ID;
    doc["TX_GIT_UNCOMMITTED_FILES_COUNT"] = GIT_UNCOMMITTED_FILES_COUNT;
    doc["TX_TRIGGER"] = txTriggerToString(tx_trigger);
    doc["TX_VBAT_RAW"] = vbat_raw;
    String output_payload;
    serializeJson(doc, output_payload);
    return output_payload;
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

/**
 * @brief Return the 64-bit board identifier in decimal form.
 * @return Board unique identifier.
 */
static inline uint64_t getBoardUidDec()
{
    static const uint64_t uid =
        ((uint64_t)NRF_FICR->DEVICEID[1] << 32) | NRF_FICR->DEVICEID[0];
    return uid;
}

/**
 * @brief Return the 64-bit board identifier in hexadecimal form.
 * @return Uppercase 16-character hexadecimal board unique identifier.
 */
static inline const char* getBoardUidHex()
{
    static char uid_hex[17];
    static bool initialized = false;

    if (initialized)
        return uid_hex;

    /**
     * @note Avoid `%llX` on embedded `printf` / nano-libc.
     * @note Format the two 32-bit words directly instead.
     */
    uint32_t uid_hi = NRF_FICR->DEVICEID[1];
    uint32_t uid_lo = NRF_FICR->DEVICEID[0];
    snprintf(
        uid_hex,
        sizeof(uid_hex),
        "%08lX%08lX",
        (unsigned long)uid_hi,
        (unsigned long)uid_lo);
    initialized = true;

    return uid_hex;
}

/**
 * @brief Enter nRF52 system-off deep sleep.
 * @see https://forum.seeedstudio.com/t/xiao-sense-accelerometer-examples-and-low-power/270801
 */
void goToDeepSleep();

uint16_t readMsgCounter();

void saveMsgCounter(uint16_t cnt);


void setupMsgCounterStorage();

uint16_t readBatteryVoltage();

void testAllLEDs();


static inline void writeRgbLeds(
    uint32_t LED_RED_STATE,
    uint32_t LED_GREEN_STATE,
    uint32_t LED_BLUE_STATE)
{
    if (settings::misc::debug)
    {
        /** @note On XIAO nRF52, `LED_BUILTIN == LED_RED`. */
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
