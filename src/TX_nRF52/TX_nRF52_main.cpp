/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <Arduino.h>
#include <nrf.h>

#include "TX_nRF52.h"
#include "common/common.h"

/**
 * @note Sleep strategy: System-ON Low Power via raw `__WFE` driven by RTC2.
 * @note The FreeRTOS-based variant tested earlier kept the chip too active
 * (TinyUSB/SoftDevice tasks defeat tickless idle) and drew ~1000x more current.
 * @note See README "TX low-power sleep strategy" for the rationale.
 */

/** @brief RTC2 prescaler producing an 8 Hz tick from the 32.768 kHz LF clock. */
static constexpr uint32_t RTC2_PRESCALER = 4095;
static constexpr uint32_t RTC2_TICKS_PER_SECOND = 8;
static constexpr uint32_t RTC2_MAX_TICKS = 0x00FFFFFFUL;

static volatile bool wakeup_pin_fired = false;
static TxTrigger tx_trigger = TxTrigger::Boot;
static uint32_t next_heartbeat_deadline_ms = 0;

extern "C" void RTC2_IRQHandler(void)
{
    NRF_RTC2->EVENTS_COMPARE[0] = 0;
}

static void onWakeupPinRise()
{
    wakeup_pin_fired = true;
}

void setupRtcWakeup()
{
    pinMode(settings::board::WAKEUP_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(settings::board::WAKEUP_PIN), onWakeupPinRise, RISING);

    NRF_RTC2->PRESCALER = RTC2_PRESCALER;
    NRF_RTC2->INTENSET = RTC_INTENSET_COMPARE0_Msk;
    NVIC_SetPriority(RTC2_IRQn, 7);
    NVIC_EnableIRQ(RTC2_IRQn);
}

/**
 * @brief Sleep up to `seconds`, optionally waking on the wakeup pin.
 * @param seconds Maximum sleep duration in seconds.
 * @param pin_enabled If true, return early when the wakeup pin rises.
 * @return true if the wakeup pin caused the wake, false on timeout.
 */
static bool sleepSecondsOrPin(uint32_t seconds, bool pin_enabled)
{
    if (pin_enabled && digitalRead(settings::board::WAKEUP_PIN))
        return true;

    wakeup_pin_fired = false;

    uint32_t ticks = seconds * RTC2_TICKS_PER_SECOND;
    if (ticks == 0)
        ticks = 1;
    if (ticks > RTC2_MAX_TICKS)
        ticks = RTC2_MAX_TICKS;

    NRF_RTC2->TASKS_STOP = 1;
    NRF_RTC2->TASKS_CLEAR = 1;
    NRF_RTC2->EVENTS_COMPARE[0] = 0;
    NRF_RTC2->CC[0] = ticks;
    NRF_RTC2->TASKS_START = 1;

    while (!NRF_RTC2->EVENTS_COMPARE[0] && !(pin_enabled && wakeup_pin_fired))
    {
        __SEV();
        __WFE();
        __WFE();
    }

    NRF_RTC2->TASKS_STOP = 1;
    return pin_enabled && wakeup_pin_fired;
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

    /** @note Short debounce window before re-checking wakeup conditions. */
    sleepSecondsOrPin(settings::misc::TX_DEBOUNCE_S, false);

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
        /** @note Wait until the next heartbeat or until the wakeup pin rises. */
        uint32_t remaining_ms = next_heartbeat_deadline_ms - now_ms;
        uint32_t sleep_seconds = (remaining_ms + 999) / 1000;
        tx_trigger = sleepSecondsOrPin(sleep_seconds, true) ? TxTrigger::WakeupPinHigh
                                                            : TxTrigger::HeartbeatTx;
    }
}
