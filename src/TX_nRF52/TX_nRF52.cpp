/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#define WAKEUP_PIN D5
// 868.0 MHz: respect legal duty-cycle constraints (typically 1%), so avoid short intervals.
#define HEARTBEAT_INTERVAL_SECONDS (20 * 60)
#define HEARTBEAT_INTERVAL_MS (HEARTBEAT_INTERVAL_SECONDS * 1000UL)

// To format the flash:
// - Set FORMAT_LITTLEFS to 1.
// - Flash the microcontroler.
// - Set FORMAT_LITTLEFS back to 0.
// - Flash the microcontroler again.
#define FORMAT_LITTLEFS 0

// If the linter does not recognize the paths below,
// or the constants like `LED_RED` :
//    -    open `user_settings.ini`
//    -    in `default_envs`
//    -    make sure `seeed_xiao_nrf52840-tx` is the first `default_envs`
#include <Arduino.h>
#include <nrf.h>
#include "../common/common_nRF52.h"
#include "../common/common.h"

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
    pinMode(WAKEUP_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(WAKEUP_PIN), onWakeupPinRise, RISING);

    if ((NRF_CLOCK->LFCLKSTAT & CLOCK_LFCLKSTAT_STATE_Msk) == 0)
    {
        NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
        NRF_CLOCK->TASKS_LFCLKSTART = 1;
        while (!NRF_CLOCK->EVENTS_LFCLKSTARTED)
            ;
    }
}

WakeupReason sleepUntilWakeupPinOrTimeout(uint32_t timeout_seconds)
{
    pinMode(WAKEUP_PIN, INPUT);
    if (digitalRead(WAKEUP_PIN))
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
    // - Must be LOW for battery voltage reading.
    // - Must be LOW during charging otherwise there is a risk of burning out the P0.31 pin.
    // - Should be HIGH during sleep to avoid current consumption.
    // https://wiki.seeedstudio.com/XIAO_BLE/#q3-what-are-the-considerations-when-using-xiao-nrf52840-sense-for-battery-charging
    pinMode(VBAT_ENABLE, OUTPUT);
    digitalWrite(VBAT_ENABLE, LOW);
}

void setup()
{
    setupGPIOs();
    setupRtcWakeup();
    nextHeartbeatDeadlineMs = millis() + HEARTBEAT_INTERVAL_MS;
    writeRgbLeds(0, 0, 1);
    setupSerial();
    setupLittleFS();
    setupLoRa();
}

void loop()
{
    writeRgbLeds(0, 1, 0);
    uint16_t battery_voltage = readBatteryVoltage();
    uint16_t cnt = readMsgCounterFromFile();

    writeRgbLeds(1, 0, 0);
    transmitLoRa(getBoardUidHex(), cnt, battery_voltage, wakeupReason);
    if (wakeupReason == WakeupReason::HeartbeatTx)
        advanceHeartbeatDeadline(millis());

    writeRgbLeds(0, 1, 0);
    saveMsgCounterToFile(++cnt);

    delay(5000 - millis() % 1000);
    pinMode(WAKEUP_PIN, INPUT);
    uint32_t now_ms = millis();
    if (isHeartbeatDue(now_ms))
    {
        wakeupReason = WakeupReason::HeartbeatTx;
        return;
    }
    if (digitalRead(WAKEUP_PIN))
    {
        wakeupReason = WakeupReason::WakeupPinHigh;
        return;
    }

    writeRgbLeds(0, 0, 0);
    uint32_t remaining_ms = nextHeartbeatDeadlineMs - now_ms;
    uint32_t sleep_seconds = (remaining_ms + 999) / 1000; // ceil(ms/1000)
    wakeupReason = sleepUntilWakeupPinOrTimeout(sleep_seconds);
}
