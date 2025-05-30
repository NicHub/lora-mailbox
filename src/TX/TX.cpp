/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <Arduino.h>
#include "common/common.h"
#include "common/LoraMailBox_Settings.h"

void goToDeepSleep()
{
    Serial.printf(PREFIX "Compilation date %s", COMPILATION_DATE);
    Serial.printf(PREFIX "Compilation time %s", COMPILATION_TIME);
    Serial.print(F(PREFIX "Going to deep sleep\n"));
    Serial.flush();
    esp_deep_sleep_start();
}

void setupDeepSleep()
{
    esp_sleep_enable_ext1_wakeup(MASK, ESP_EXT1_WAKEUP_ANY_HIGH);
}

void debounce(uint32_t wait)
{
    delay(wait - millis() % 1000);
}

void transmitLoRa(uint16_t cnt)
{
    String msg;
    JsonDocument doc;
    doc["cnt"] = cnt;
    serializeJson(doc, msg);

    Serial.printf(PREFIX "Sending\t\t%s", msg.c_str());
    int state = radio.startTransmit(msg.c_str());
    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf(PREFIX "Failed, code %d", state);
        return;
    }
    Serial.print(F(PREFIX "Transmission finished!"));
}

void setupGPIOs()
{
    // Beware that if you use XIAO ESP32S3 with LoRa
    // shield SX1262, LORA_USER_BUTTON on LoRa shield
    // and BUILTIN_LED on ESP module are both connected
    // on GPIO21! So if you set BUILTIN_LED pinMode to
    // OUTPUT and you set BUILTIN_LED to HIGH and you
    // press LORA_USER_BUTTON, you create a
    // short-circuit between GPIIO21 and GND.
    pinMode(LORA_USER_BUTTON, INPUT);
    pinMode(LORA_GREEN_LED, OUTPUT);

    // PIR
    pinMode(PIR_PIN_0_VCC, OUTPUT);
    pinMode(PIR_PIN_0, INPUT);
    pinMode(PIR_PIN_0_GND, OUTPUT);
    digitalWrite(PIR_PIN_0_VCC, HIGH);
    digitalWrite(PIR_PIN_0_GND, LOW);
}

void setup()
{
    setupGPIOs();
    setupSerial();
    setupLittleFS();
    setupLoRa();
}

void loop()
{
    uint16_t cnt = readMsgCounterFromFile();
    transmitLoRa(cnt);
    saveMsgCounterToFile(++cnt);
    debounce(5000);

    // Set DEEP_SLEEP to false to send messages
    // continuously for example to perform signal
    // quality tests.
#define DEEP_SLEEP true
#if DEEP_SLEEP
    setupDeepSleep();
    goToDeepSleep();
#endif

    // If the module goes to deep sleep, the code below
    // is never executed because the ESP restarts upon
    // waking up.
    blink();
    debounce(1000);
}
