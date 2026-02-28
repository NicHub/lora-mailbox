/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#define BOARD_ID 1
#define WAKEUP_PIN D5

// To format the flash:
// - Set FORMAT_LITTLEFS to 1.
// - Flash the microcontroler.
// - Set FORMAT_LITTLEFS back to 0.
// - Flash the microcontroler again.
#define FORMAT_LITTLEFS 0

// If the linter does not recognize the paths below,
// or the constants like `LED_RED` :
//    -    open `platformio_user_preferences.ini`
//    -    in `default_envs`
//    -    make sure `seeed_xiao_nrf52840-tx` is the first `default_envs`
#include <Arduino.h>
#include "../common/common_nRF52.h"
#include "../common/common.h"

#if DEBUG
namespace
{
constexpr uint32_t DEBUG_LED_PULSE_MS = 100;

SoftwareTimer debugLedTimer;
volatile int32_t activeDebugLedPin = -1;

void debugLedOffCallback(TimerHandle_t)
{
    int32_t ledPin = activeDebugLedPin;
    if (ledPin < 0)
        return;

    digitalWrite(ledPin, HIGH);
    activeDebugLedPin = -1;
}

void setupDebugLedTimer()
{
    debugLedTimer.begin(DEBUG_LED_PULSE_MS, debugLedOffCallback, nullptr, false);
}

void debugPulseLed(uint32_t ledPin)
{
    debugLedTimer.stop();
    switchOffAllLEDs();
    activeDebugLedPin = static_cast<int32_t>(ledPin);
    digitalWrite(ledPin, LOW);
    debugLedTimer.start();
}
} // namespace
#endif

void setupGPIOs()
{
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    switchOffAllLEDs();

    // PIN_CHARGING_CURRENT
    // - LOW: High charging current (100 mA)
    // - HIGH: Low charging current (50 mA)
    pinMode(PIN_CHARGING_CURRENT, OUTPUT);
    digitalWrite(PIN_CHARGING_CURRENT, HIGH);
}

void setup()
{
    setupGPIOs();
#if DEBUG
    setupDebugLedTimer();
    debugPulseLed(LED_BLUE);
#endif
    setupSerial();
    // while (true)
    //     testAllLEDs();
    setupLittleFS();
    setupLoRa();
}

void loop()
{
#if DEBUG
    debugPulseLed(LED_GREEN);
#endif
    uint16_t cnt = readMsgCounterFromFile();
    uint16_t battery_voltage = readBatteryVoltage();

#if DEBUG
    debugPulseLed(LED_RED);
#endif
    transmitLoRa(BOARD_ID, cnt, battery_voltage);

#if DEBUG
    debugPulseLed(LED_GREEN);
#endif
    saveMsgCounterToFile(++cnt);
    delay(5000 - millis() % 1000);

    pinMode(WAKEUP_PIN, INPUT);
    if (digitalRead(WAKEUP_PIN))
        return;

#if DEBUG
    switchOffAllLEDs();
#endif
    goToDeepSleep();
}
