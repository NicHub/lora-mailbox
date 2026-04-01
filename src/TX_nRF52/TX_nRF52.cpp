/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

// If the linter does not recognize the paths below,
// or the constants like `LED_RED` :
//    -    open `user_settings.ini`
//    -    in `default_envs`
//    -    make sure `seeed_xiao_nrf52840-tx` is the first `default_envs`
#include <Arduino.h>
#include <nrf.h>
#include "common/common_nRF52.h"
#include "common/common.h"

static volatile bool rtcTimeoutElapsed = false;
static volatile bool wakeupPinEvent = false;
static WakeupReason wakeupReason = WakeupReason::Boot;
static uint32_t nextHeartbeatDeadlineMs = 0;

static constexpr uint32_t RTC_PRESCALER = 4095; // 32768 / (4095 + 1) = 8 Hz
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
    pinMode(board::hw::wakeup_pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(board::hw::wakeup_pin), onWakeupPinRise, RISING);

    if ((NRF_CLOCK->LFCLKSTAT & CLOCK_LFCLKSTAT_STATE_Msk) == 0)
    {
        NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
        NRF_CLOCK->TASKS_LFCLKSTART = 1;
        while (!NRF_CLOCK->EVENTS_LFCLKSTARTED)
            ;
    }
}

// Low-power sleep using RTC, without pin wakeup (unconditional wait).
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

WakeupReason sleepUntilWakeupPinOrTimeout(uint32_t timeout_seconds)
{
    pinMode(board::hw::wakeup_pin, INPUT);
    if (digitalRead(board::hw::wakeup_pin))
        return WakeupReason::WakeupPinHigh;

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
        return WakeupReason::WakeupPinHigh;
    return WakeupReason::HeartbeatTx;
}

static inline bool isHeartbeatDue(uint32_t now_ms)
{
    return (int32_t)(now_ms - nextHeartbeatDeadlineMs) >= 0;
}

static void advanceHeartbeatDeadline(uint32_t now_ms)
{
    // Keep a fixed heartbeat cadence, independent of processing jitter.
    do
    {
        nextHeartbeatDeadlineMs += HEARTBEAT_INTERVAL_MS;
    } while ((int32_t)(now_ms - nextHeartbeatDeadlineMs) >= 0);
}

void setupGPIOs()
{
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    writeRgbLeds(0, 0, 0);

    // PIN_CHARGING_CURRENT
    // - LOW: High charging current (100 mA)
    // - HIGH: Low charging current (50 mA)
    pinMode(PIN_CHARGING_CURRENT, OUTPUT);
    digitalWrite(PIN_CHARGING_CURRENT, HIGH);

    // VBAT_ENABLE
    // See comment in the function readBatteryVoltage().
    pinMode(VBAT_ENABLE, INPUT);
}

void setup()
{
    setupGPIOs();
    writeRgbLeds(0, 0, 1);
    setupRtcWakeup();
    nextHeartbeatDeadlineMs = millis() + HEARTBEAT_INTERVAL_MS;
    setupSerial();
    setupMsgCounterStorage();
    if (TX_RESET_MSG_COUNTER_ON_REBOOT)
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

    writeRgbLeds(1, 0, 0);
    transmitLoRa(getBoardUidHex(), cnt, battery_voltage, wakeupReason);
    if (wakeupReason == WakeupReason::HeartbeatTx)
        advanceHeartbeatDeadline(millis());

    // Put radio to sleep immediately after transmission.
    // The SX1262 consumes ~1.6 mA in standby vs ~0.9 µA in sleep.
    // RadioLib wakes it automatically on the next transmit() call (NSS falling edge).
    radio.sleep();

    // Flush serial before sleeping to avoid UART current draw on buffered data.
    Serial.flush();

    writeRgbLeds(0, 1, 0);
    saveMsgCounter(++cnt);

    // Keep a short debounce window before re-checking wakeup conditions.
    sleepSecondsNoPin(TX_DEBOUNCE_S);

    pinMode(board::hw::wakeup_pin, INPUT);
    uint32_t now_ms = millis();
    if (isHeartbeatDue(now_ms))
    {
        wakeupReason = WakeupReason::HeartbeatTx;
        return;
    }
    if (digitalRead(board::hw::wakeup_pin))
    {
        wakeupReason = WakeupReason::WakeupPinHigh;
        return;
    }

    writeRgbLeds(0, 0, 0);
    uint32_t remaining_ms = nextHeartbeatDeadlineMs - now_ms;
    uint32_t sleep_seconds = (remaining_ms + 999) / 1000; // ceil(ms/1000)
    wakeupReason = sleepUntilWakeupPinOrTimeout(sleep_seconds);
}
