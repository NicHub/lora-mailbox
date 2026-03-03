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
    transmitLoRa(BOARD_ID, cnt, battery_voltage);

    writeRgbLeds(0, 1, 0);
    saveMsgCounterToFile(++cnt);
    delay(5000 - millis() % 1000);

    pinMode(WAKEUP_PIN, INPUT);
    if (digitalRead(WAKEUP_PIN))
        return;

    writeRgbLeds(0, 0, 0);
    goToDeepSleep();
}
