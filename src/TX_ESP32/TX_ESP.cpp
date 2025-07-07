/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#define BOARD_ID 0
#define WAKEUP_PIN D5

// To format the flash:
// - Set FORMAT_LITTLEFS to 1.
// - Flash the microcontroler.
// - Set FORMAT_LITTLEFS back to 0.
// - Flash the microcontroler again.
#define FORMAT_LITTLEFS 0

#include <Arduino.h>
#include "../common/common_ESP32.h"
#include "../common/common.h"

void setupGPIOs()
{
    // Beware that if you use XIAO ESP32S3 with LoRa
    // shield SX1262, LORA_USER_BUTTON on LoRa shield
    // and LED_BUILTIN on ESP module are both connected
    // on GPIO21! So if you set LED_BUILTIN pinMode to
    // OUTPUT and you set LED_BUILTIN to HIGH and you
    // press LORA_USER_BUTTON, you create a
    // short-circuit between GPIIO21 and GND.

    // pinMode(LED_BUILTIN, OUTPUT); => Too dangerous to use!
    pinMode(LORA_USER_BUTTON, INPUT);
    pinMode(LORA_LED_GREEN, OUTPUT);
    pinMode(WAKEUP_PIN, INPUT);
}

void setup()
{
    setupGPIOs();
    setupSerial();
    for (size_t i = 0; i < 4; i++)
    {
        delay(1000);
        Serial.println(i);
    }
    setupLittleFS();
    setupLoRa();
}

void loop()
{
    uint16_t cnt = readMsgCounterFromFile();
    saveMsgCounterToFile(++cnt);
    uint16_t battery_voltage = readBatteryVoltage();
    transmitLoRa(BOARD_ID, cnt, battery_voltage);
    delay(3000);
    if (!digitalRead(WAKEUP_PIN))
        goToDeepSleep();
}
