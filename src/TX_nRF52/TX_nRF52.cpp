/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <Arduino.h>
#include <FreeRTOS.h>
#include <nrf.h>
#include <semphr.h>

#include "nRF52.h"
#include "common/common.h"

static SemaphoreHandle_t wakeup_sem;
static TxTrigger tx_trigger = TxTrigger::Boot;
static uint32_t next_heartbeat_deadline_ms = 0;

void onWakeupPinRise()
{
    BaseType_t higher_priority_woken = pdFALSE;
    xSemaphoreGiveFromISR(wakeup_sem, &higher_priority_woken);
    portYIELD_FROM_ISR(higher_priority_woken);
}

void setupRtcWakeup()
{
    wakeup_sem = xSemaphoreCreateBinary();
    pinMode(settings::board::WAKEUP_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(settings::board::WAKEUP_PIN), onWakeupPinRise, RISING);
}

static inline bool isHeartbeatDue(uint32_t now_ms)
{
    return (int32_t)(now_ms - next_heartbeat_deadline_ms) >= 0;
}

static void advanceHeartbeatDeadline(uint32_t now_ms)
{
    /** @note Keep a fixed heartbeat cadence, independent of processing jitter. */
    do
    {
        next_heartbeat_deadline_ms += settings::misc::TX_HEARTBEAT_INTERVAL_MS;
    } while ((int32_t)(now_ms - next_heartbeat_deadline_ms) >= 0);
}

void setupGPIOs()
{
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    writeRgbLeds(0, 0, 0);

    /**
     * @brief Configure charger current selection.
     * @note `LOW` selects high charging current (100 mA).
     * @note `HIGH` selects low charging current (50 mA).
     */
    pinMode(PIN_CHARGING_CURRENT, OUTPUT);
    digitalWrite(PIN_CHARGING_CURRENT, HIGH);

    /** @note See `readBatteryVoltage()` and the hardware notes for `VBAT_ENABLE`. */
    pinMode(VBAT_ENABLE, INPUT);
}

void setup()
{
    setupGPIOs();
    writeRgbLeds(0, 0, 1);
    setupRtcWakeup();
    next_heartbeat_deadline_ms = millis() + settings::misc::TX_HEARTBEAT_INTERVAL_MS;
    setupSerial();
    setupMsgCounterStorage();
    if (settings::misc::TX_RESET_MSG_COUNTER_ON_REBOOT)
    {
        saveMsgCounter(0);
        Serial.println("MSG COUNTER RESET TO 0 (TX_RESET_MSG_COUNTER_ON_REBOOT=true)");
    }
    setupLoRa();
}

void loop()
{
    writeRgbLeds(0, 1, 0);
    uint16_t battery_voltage = readBatteryVoltage();
    uint16_t cnt = readMsgCounter();
    String payload = buildTxPayload(getBoardUidHex(), cnt, battery_voltage, tx_trigger);

    writeRgbLeds(1, 0, 0);
    sendLoRaPayload(payload.c_str());
    if (tx_trigger == TxTrigger::HeartbeatTx)
        advanceHeartbeatDeadline(millis());

    /**
     * @note Put the radio to sleep immediately after transmission.
     * @note The SX1262 consumes about 1.6 mA in standby versus about 0.9 uA in sleep.
     * @note RadioLib wakes it automatically on the next `transmit()` call.
     */
    radio.sleep();

    /** @note Flush serial before sleeping to avoid UART current draw on buffered data. */
    Serial.flush();

    writeRgbLeds(0, 1, 0);
    saveMsgCounter(++cnt);

    writeRgbLeds(0, 0, 0);

    /** @note Keep a short debounce window before re-checking wakeup conditions. */
    delay(settings::misc::TX_DEBOUNCE_S * 1000);

    uint32_t now_ms = millis();
    if (isHeartbeatDue(now_ms))
    {
        tx_trigger = TxTrigger::HeartbeatTx;
    }
    else if (digitalRead(settings::board::WAKEUP_PIN))
    {
        tx_trigger = TxTrigger::WakeupPinHigh;
    }
    else
    {
        /** @note Wait exactly until the next heartbeat or until an interrupt wakes us. */
        uint32_t sleep_ms = next_heartbeat_deadline_ms - now_ms;
        xSemaphoreTake(wakeup_sem, 0); // Clear any pending events
        if (xSemaphoreTake(wakeup_sem, pdMS_TO_TICKS(sleep_ms)) == pdTRUE)
            tx_trigger = TxTrigger::WakeupPinHigh;
        else
            tx_trigger = TxTrigger::HeartbeatTx;
    }
}
