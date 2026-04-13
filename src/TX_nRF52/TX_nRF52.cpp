/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <Arduino.h>
#include <nrf.h>
#include "common/common_nRF52.h"
#include "common/common.h"

#include <FreeRTOS.h>
#include <semphr.h>

static SemaphoreHandle_t wakeupSem;
static TxTrigger txTrigger = TxTrigger::Boot;
static uint32_t nextHeartbeatDeadlineMs = 0;

void onWakeupPinRise()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(wakeupSem, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void setupRtcWakeup()
{
    wakeupSem = xSemaphoreCreateBinary();
    pinMode(settings::board::wakeup_pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(settings::board::wakeup_pin), onWakeupPinRise, RISING);
}



static inline bool isHeartbeatDue(uint32_t now_ms)
{
    return (int32_t)(now_ms - nextHeartbeatDeadlineMs) >= 0;
}

static void advanceHeartbeatDeadline(uint32_t now_ms)
{
    /** @note Keep a fixed heartbeat cadence, independent of processing jitter. */
    do
    {
        nextHeartbeatDeadlineMs += settings::misc::tx_heartbeat_interval_ms;
    } while ((int32_t)(now_ms - nextHeartbeatDeadlineMs) >= 0);
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
    nextHeartbeatDeadlineMs = millis() + settings::misc::tx_heartbeat_interval_ms;
    setupSerial();
    setupMsgCounterStorage();
    if (settings::misc::tx_reset_msg_counter_on_reboot)
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
    String payload = buildTxPayload(getBoardUidHex(), cnt, battery_voltage, txTrigger);

    writeRgbLeds(1, 0, 0);
    sendLoRaPayload(payload.c_str());
    if (txTrigger == TxTrigger::HeartbeatTx)
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
    delay(settings::misc::tx_debounce_s * 1000);

    uint32_t now_ms = millis();
    if (isHeartbeatDue(now_ms))
    {
        txTrigger = TxTrigger::HeartbeatTx;
    }
    else if (digitalRead(settings::board::wakeup_pin))
    {
        txTrigger = TxTrigger::WakeupPinHigh;
    }
    else
    {
        /** @note Wait exactly until the next heartbeat or until an interrupt wakes us. */
        uint32_t sleep_ms = nextHeartbeatDeadlineMs - now_ms;
        xSemaphoreTake(wakeupSem, 0); // Clear any pending events
        if (xSemaphoreTake(wakeupSem, pdMS_TO_TICKS(sleep_ms)) == pdTRUE)
            txTrigger = TxTrigger::WakeupPinHigh;
        else
            txTrigger = TxTrigger::HeartbeatTx;
    }
}
