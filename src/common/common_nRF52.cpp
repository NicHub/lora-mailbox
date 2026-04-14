/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#if defined(NRF52_SERIES)

#include "common/common_nRF52.h"
#include "common/common.h"

void goToDeepSleep()
{
    pinMode(settings::board::wakeup_pin, INPUT_SENSE_HIGH);
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

void saveMsgCounter(uint16_t cnt)
{
    uint32_t t1 = millis();
    int32_t last_record_index = getLastCounterRecordIndex();
    uint32_t next_record_index = static_cast<uint32_t>(last_record_index + 1);
    if (next_record_index >= counterRecordCapacity())
    {
        if (!eraseCounterStorage())
        {
            Serial.println("Counter storage erase failed");
            return;
        }
        next_record_index = 0;
        counter_record_index_cache = -1;
    }

    CounterRecord record{
        .counter = cnt,
        .checksum = counterChecksum(cnt),
        .magic = COUNTER_RECORD_MAGIC,
    };
    if (flash_nrf5x_write(counterRecordAddress(next_record_index), &record, sizeof(record)) < 0)
    {
        Serial.println("Counter storage write failed");
        return;
    }
    flash_nrf5x_flush();
    counter_record_index_cache = static_cast<int32_t>(next_record_index);

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

#endif // NRF52_SERIES
