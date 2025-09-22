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

#include <Arduino.h>
#include "../common/common_nRF52.h"
#include "../common/common.h"

void setupGPIOs()
{
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
    digitalWrite(LED_BLUE, LOW);
#endif
    setupSerial();
    setupLittleFS();
    setupLoRa();
    switchOffAllLEDs();
}

void loop()
{
    uint16_t cnt = readMsgCounterFromFile();
    uint16_t battery_voltage = readBatteryVoltage();
    transmitLoRa(BOARD_ID, cnt, battery_voltage);
    saveMsgCounterToFile(++cnt);
    delay(5000 - millis() % 1000);
    blink(10, 100, 5, LED_GREEN, true);
    if (!digitalRead(WAKEUP_PIN))
        return;
    goToDeepSleep();
}
