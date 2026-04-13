/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <Arduino.h>
#include <nrf.h>
#include "common/common_nRF52.h"
#include "common/common.h"

static volatile bool rtcTimeoutElapsed = false;
static volatile bool wakeupPinEvent = false;
static TxTrigger txTrigger = TxTrigger::Boot;
static uint32_t nextHeartbeatDeadlineMs = 0;

/** @brief RTC prescaler for an 8 Hz tick from the 32.768 kHz LF clock. */
static constexpr uint32_t RTC_PRESCALER = 4095;
static constexpr uint32_t RTC_TICKS_PER_SECOND = 8;

extern "C" void RTC2_IRQHandler(void)
{
    if (NRF_RTC2->EVENTS_COMPARE[0])
    {
        NRF_RTC2->EVENTS_COMPARE[0] = 0;
        rtcTimeoutElapsed = true;
    }
}

void onWakeupPinRise()
{
    wakeupPinEvent = true;
}

void setupRtcWakeup()
{
    pinMode(settings::board::wakeup_pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(settings::board::wakeup_pin), onWakeupPinRise, RISING);

    if ((NRF_CLOCK->LFCLKSTAT & CLOCK_LFCLKSTAT_STATE_Msk) == 0)
    {
        NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
        NRF_CLOCK->TASKS_LFCLKSTART = 1;
        while (!NRF_CLOCK->EVENTS_LFCLKSTARTED)
            ;
    }
}

/**
 * @brief Sleep for a fixed number of seconds using RTC2 only.
 * @param seconds Sleep duration in seconds.
 *
 * @note This path does not wake on the external wakeup pin.
 */
static void sleepSecondsNoPin(uint32_t seconds)
{
    uint32_t ticks = seconds * RTC_TICKS_PER_SECOND;
    if (ticks == 0)
        ticks = 1;
    if (ticks > 0x00FFFFFFUL)
        ticks = 0x00FFFFFFUL;

    rtcTimeoutElapsed = false;
    NRF_RTC2->TASKS_STOP = 1;
    NRF_RTC2->TASKS_CLEAR = 1;
    NRF_RTC2->PRESCALER = RTC_PRESCALER;
    NRF_RTC2->CC[0] = ticks;
    NRF_RTC2->EVENTS_COMPARE[0] = 0;
    NRF_RTC2->INTENSET = RTC_INTENSET_COMPARE0_Msk;
    NVIC_ClearPendingIRQ(RTC2_IRQn);
    NVIC_EnableIRQ(RTC2_IRQn);
    NRF_RTC2->TASKS_START = 1;

    while (!rtcTimeoutElapsed)
    {
        __SEV();
        __WFE();
        __WFE();
    }

    NRF_RTC2->TASKS_STOP = 1;
    NRF_RTC2->INTENCLR = RTC_INTENCLR_COMPARE0_Msk;
}

TxTrigger sleepUntilWakeupPinOrTimeout(uint32_t timeout_seconds)
{
    pinMode(settings::board::wakeup_pin, INPUT);
    if (digitalRead(settings::board::wakeup_pin))
        return TxTrigger::WakeupPinHigh;

    rtcTimeoutElapsed = false;
    wakeupPinEvent = false;

    uint32_t timeout_ticks = timeout_seconds * RTC_TICKS_PER_SECOND;
    if (timeout_ticks == 0)
        timeout_ticks = 1;
    if (timeout_ticks > 0x00FFFFFFUL)
        timeout_ticks = 0x00FFFFFFUL;

    NRF_RTC2->TASKS_STOP = 1;
    NRF_RTC2->TASKS_CLEAR = 1;
    NRF_RTC2->PRESCALER = RTC_PRESCALER;
    NRF_RTC2->CC[0] = timeout_ticks;
    NRF_RTC2->EVENTS_COMPARE[0] = 0;
    NRF_RTC2->INTENSET = RTC_INTENSET_COMPARE0_Msk;
    NVIC_ClearPendingIRQ(RTC2_IRQn);
    NVIC_EnableIRQ(RTC2_IRQn);
    NRF_RTC2->TASKS_START = 1;

    while (!rtcTimeoutElapsed && !wakeupPinEvent)
    {
        __SEV();
        __WFE();
        __WFE();
    }

    NRF_RTC2->TASKS_STOP = 1;
    NRF_RTC2->INTENCLR = RTC_INTENCLR_COMPARE0_Msk;
    if (wakeupPinEvent)
        return TxTrigger::WakeupPinHigh;
    return TxTrigger::HeartbeatTx;
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
    sendLoRaPayload(payload);
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

    /** @note Keep a short debounce window before re-checking wakeup conditions. */
    sleepSecondsNoPin(settings::misc::tx_debounce_s);

    pinMode(settings::board::wakeup_pin, INPUT);
    uint32_t now_ms = millis();
    if (isHeartbeatDue(now_ms))
    {
        txTrigger = TxTrigger::HeartbeatTx;
        return;
    }
    if (digitalRead(settings::board::wakeup_pin))
    {
        txTrigger = TxTrigger::WakeupPinHigh;
        return;
    }

    writeRgbLeds(0, 0, 0);
    uint32_t remaining_ms = nextHeartbeatDeadlineMs - now_ms;
    /** @note Round up milliseconds to whole seconds before entering RTC sleep. */
    uint32_t sleep_seconds = (remaining_ms + 999) / 1000;
    txTrigger = sleepUntilWakeupPinOrTimeout(sleep_seconds);
}
