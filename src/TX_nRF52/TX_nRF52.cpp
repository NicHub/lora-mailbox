/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include "TX_nRF52.h"
#include "common/common.h"

void goToDeepSleep()
{
    pinMode(settings::board::WAKEUP_PIN, INPUT_SENSE_HIGH);
    debounce(1000);
    NRF_POWER->SYSTEMOFF = 1;
}

uint16_t readMsgCounter()
{
    int32_t last_record_index = getLastCounterRecordIndex();
    if (last_record_index < 0)
        return 0;
    return readCounterRecord(static_cast<uint32_t>(last_record_index)).counter;
}

uint16_t readVInitialRaw()
{
    int32_t last_record_index = getLastCounterRecordIndex();
    if (last_record_index < 0)
        return 0;
    return readCounterRecord(static_cast<uint32_t>(last_record_index)).v_initial_raw;
}

static bool writeCounterRecord(uint16_t cnt, uint16_t v_initial_raw)
{
    int32_t last_record_index = getLastCounterRecordIndex();
    uint32_t next_record_index = static_cast<uint32_t>(last_record_index + 1);
    if (next_record_index >= counterRecordCapacity())
    {
        if (!eraseCounterStorage())
        {
            Serial.println("Counter storage erase failed");
            return false;
        }
        next_record_index = 0;
        counter_record_index_cache = -1;
    }

    CounterRecord record{
        .magic = COUNTER_RECORD_MAGIC,
        .build_id = BUILD_ID,
        .counter = cnt,
        .v_initial_raw = v_initial_raw,
        .checksum = counterChecksum(BUILD_ID, cnt, v_initial_raw),
    };
    if (flash_nrf5x_write(counterRecordAddress(next_record_index), &record, sizeof(record)) < 0)
    {
        Serial.println("Counter storage write failed");
        return false;
    }
    flash_nrf5x_flush();
    counter_record_index_cache = static_cast<int32_t>(next_record_index);
    return true;
}

void saveMsgCounter(uint16_t cnt)
{
    uint32_t t1 = millis();
    /** @note Preserve v_initial_raw across saves; it is only (re)captured on flash. */
    uint16_t v_initial_raw = readVInitialRaw();
    if (!writeCounterRecord(cnt, v_initial_raw))
        return;

    Serial.print("\n\nmillis(), Time elapsed to saveMsgCounter = ");
    Serial.print(millis());
    Serial.print("\t");
    Serial.print(millis() - t1);
}

void setupMsgCounterStorage()
{
    counter_record_index_cache = findLastCounterRecordIndex();
    int32_t last_record_index = counter_record_index_cache;
    if (last_record_index >= 0)
    {
        CounterRecord record = readCounterRecord(static_cast<uint32_t>(last_record_index));
        if (record.build_id == BUILD_ID)
            return;
        Serial.println("New BUILD_ID detected: resetting counter and capturing v_initial_raw");
    }

    if (!eraseCounterStorage())
    {
        Serial.println("Counter storage init failed");
        blink(10, 100, 10, LED_RED, true);
        return;
    }
    counter_record_index_cache = -1;
    uint16_t v_initial_raw = readBatteryVoltage();
    if (!writeCounterRecord(0, v_initial_raw))
    {
        blink(10, 100, 10, LED_RED, true);
        return;
    }
    if (readMsgCounter() != 0 || readVInitialRaw() != v_initial_raw)
    {
        Serial.println("Counter storage verification failed");
        blink(10, 100, 10, LED_RED, true);
    }
}

uint16_t readBatteryVoltage()
{
    /** @note
     * Hardware details and operating
     * constraints are documented in
     * docs/xiao-nrf52840-sense-hardware-notes.md.
     */

    pinMode(VBAT_ENABLE, OUTPUT);
    digitalWrite(VBAT_ENABLE, LOW);
    pinMode(PIN_VBAT, INPUT);

    /** @note Let the analog node settle before sampling. */
    delay(2);
    /** @note Discard the first sample, which is often dirty after switching. */
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

    /**
     * @note Reset `VBAT_ENABLE` to `INPUT` after reading
     * to avoid current draw through the resistor divider.
     */
    pinMode(VBAT_ENABLE, INPUT);
    return vbat;
}

void testAllLEDs()
{
    /** @note
     * Hardware details and operating
     * constraints are documented in
     * docs/xiao-nrf52840-sense-hardware-notes.md.
     */

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
