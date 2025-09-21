/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

/*

Deep sleep from
https://forum.seeedstudio.com/t/xiao-sense-accelerometer-examples-and-low-power/270801

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
    pinMode(WAKEUP_PIN, INPUT_SENSE_HIGH);
    pinMode(VBAT_ENABLE, OUTPUT);
    digitalWrite(VBAT_ENABLE, LOW);
    pinMode(PIN_VBAT, INPUT);
}

void setup()
{
    setupGPIOs();
    digitalWrite(LED_BLUE, LOW);
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
        goToDeepSleep();
}
