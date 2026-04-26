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

#ifndef BUILD_ID
#define BUILD_ID 0u
#endif

/**
 * @brief Persisted message counter and battery baseline.
 * @note `v_initial_raw` is captured the first time a record is written for a
 * given `BUILD_ID` (i.e. on a fresh flash) and copied unchanged on every
 * subsequent save, so the consumer can derive battery drop per message.
 */
struct CounterRecord
{
    uint32_t magic;
    uint32_t build_id;
    uint16_t counter;
    uint16_t v_initial_raw;
    uint32_t checksum;
};

static_assert(sizeof(CounterRecord) == 16, "CounterRecord must stay 16 bytes");

/** @note Bumped from 0x4D424358 when the record layout changed. */
static constexpr uint32_t COUNTER_RECORD_MAGIC = 0x4D424359UL;
static int32_t counter_record_index_cache = -2;

static inline uint32_t counterChecksum(uint32_t build_id, uint16_t counter, uint16_t v_initial_raw)
{
    return COUNTER_RECORD_MAGIC ^ build_id ^
           (static_cast<uint32_t>(counter) << 16) ^ v_initial_raw;
}

static inline bool isCounterRecordValid(const CounterRecord &record)
{
    return record.magic == COUNTER_RECORD_MAGIC &&
           record.checksum == counterChecksum(record.build_id, record.counter, record.v_initial_raw);
}

static inline uint32_t counterRecordCapacity()
{
    return COUNTER_STORAGE_SIZE / sizeof(CounterRecord);
}

static inline uint32_t counterRecordAddress(uint32_t index)
{
    return COUNTER_STORAGE_ADDR + index * sizeof(CounterRecord);
}

static inline String
buildTxPayload(const char *board_id, uint16_t cnt, uint16_t vbat_raw, uint16_t v_initial_raw, TxTrigger tx_trigger)
{
    /** @note Signed delta: a negative value signals a battery change (new cell fuller than baseline). */
    float drop_per_msg = (cnt > 0)
        ? static_cast<float>(static_cast<int32_t>(v_initial_raw) - static_cast<int32_t>(vbat_raw)) / cnt
        : 0.0f;

    JsonDocument doc;
    doc["TX_BOARD_ID"] = board_id;
    doc["TX_COUNTER"] = cnt;
    doc["TX_DEBUG"] = settings::misc::DEBUG;
    doc["TX_GIT_HEAD_COMMIT_ID"] = GIT_HEAD_COMMIT_ID;
    doc["TX_GIT_UNCOMMITTED_FILES_COUNT"] = GIT_UNCOMMITTED_FILES_COUNT;
    doc["TX_TRIGGER"] = txTriggerToString(tx_trigger);
    doc["TX_VBAT_RAW"] = vbat_raw;
    doc["TX_VBAT_DROP_PER_MSG_RAW"] = drop_per_msg;
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
    for (uint32_t addr = COUNTER_STORAGE_ADDR; addr < COUNTER_STORAGE_ADDR + COUNTER_STORAGE_SIZE;
         addr += FLASH_NRF52_PAGE_SIZE)
    {
        if (!flash_nrf5x_erase(addr))
            return false;
    }
    flash_nrf5x_flush();
    return true;
}

static inline int32_t findLastCounterRecordIndex()
{
    int32_t last_valid_index = -1;
    for (uint32_t index = 0; index < counterRecordCapacity(); ++index)
    {
        CounterRecord record = readCounterRecord(index);
        if (record.magic == 0xFFFFFFFFUL)
            break;
        if (!isCounterRecordValid(record))
            break;
        last_valid_index = static_cast<int32_t>(index);
    }
    return last_valid_index;
}

static inline int32_t getLastCounterRecordIndex()
{
    if (counter_record_index_cache == -2)
        counter_record_index_cache = findLastCounterRecordIndex();
    return counter_record_index_cache;
}

/**
 * @brief Return the 64-bit board identifier in decimal form.
 * @return Board unique identifier.
 */
static inline uint64_t getBoardUidDec()
{
    static const uint64_t UID = ((uint64_t)NRF_FICR->DEVICEID[1] << 32) | NRF_FICR->DEVICEID[0];
    return UID;
}

/**
 * @brief Return the 64-bit board identifier in hexadecimal form.
 * @return Uppercase 16-character hexadecimal board unique identifier.
 */
static inline const char *getBoardUidHex()
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
    snprintf(uid_hex, sizeof(uid_hex), "%08lX%08lX", (unsigned long)uid_hi, (unsigned long)uid_lo);
    initialized = true;

    return uid_hex;
}

/**
 * @brief Enter nRF52 system-off deep sleep.
 * @see https://forum.seeedstudio.com/t/xiao-sense-accelerometer-examples-and-low-power/270801
 */
void goToDeepSleep();

extern TxTrigger tx_trigger;

extern uint32_t next_heartbeat_deadline_ms;

void setupGPIOs();

void setupRtcWakeup();

bool sleepSecondsOrPin(uint32_t seconds, bool pin_enabled);

bool isHeartbeatDue(uint32_t now_ms);

void advanceHeartbeatDeadline(uint32_t now_ms);

uint16_t readMsgCounter();

uint16_t readBatteryRawInitial();

void saveMsgCounter(uint16_t cnt);

void setupMsgCounterStorage();

uint16_t readBatteryVoltage();

void testAllLEDs();

static inline void
writeRgbLeds(uint32_t led_red_state, uint32_t led_green_state, uint32_t led_blue_state)
{
    if (settings::misc::DEBUG)
    {
        /** @note On XIAO nRF52, `LED_BUILTIN == LED_RED`. */
        digitalWrite(LED_RED, !led_red_state);
        digitalWrite(LED_GREEN, !led_green_state);
        digitalWrite(LED_BLUE, !led_blue_state);
    }
    else
    {
        (void)led_red_state;
        (void)led_green_state;
        (void)led_blue_state;
    }
}
