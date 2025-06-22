/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <Arduino.h>
#include "common/common.h"
#include "common/LoraMailBox_Settings.h"

void setupDeepSleep()
{
    esp_sleep_enable_ext1_wakeup(MASK, ESP_EXT1_WAKEUP_ANY_HIGH);
}

void goToDeepSleep()
{
    Serial.printf(PREFIX "Compilation date %s", COMPILATION_DATE);
    Serial.printf(PREFIX "Compilation time %s", COMPILATION_TIME);
    Serial.print(F(PREFIX "Going to deep sleep\n"));
    Serial.flush();

    setupDeepSleep();
    debounce(1000);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LORA_LED_GREEN, LOW);
    esp_deep_sleep_start();
}

void stayAwake()
{
    debounce(1000);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LORA_LED_GREEN, LOW);
}

void transmitLoRa(uint16_t cnt)
{
    String msg;
    JsonDocument doc;
    doc["cnt"] = cnt;
    serializeJson(doc, msg);

    Serial.printf(PREFIX "Sending\t\t%s", msg.c_str());

    // Don’t use the non-blocking `startTransmit()` function.
    // It makes it difficult to estimate the necessary delay
    // before sending a new message.
    digitalWrite(LORA_LED_GREEN, HIGH);
    digitalWrite(LED_BUILTIN, LOW);
    int state = radio.transmit(msg.c_str());
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LORA_LED_GREEN, LOW);

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
    // and LED_BUILTIN on ESP module are both connected
    // on GPIO21! So if you set LED_BUILTIN pinMode to
    // OUTPUT and you set LED_BUILTIN to HIGH and you
    // press LORA_USER_BUTTON, you create a
    // short-circuit between GPIIO21 and GND.
    pinMode(LORA_USER_BUTTON, INPUT);
    pinMode(LORA_LED_GREEN, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    // switchOffAllLEDs();

    pinMode(WAKEUP_PIN, INPUT);
    pinMode(STAY_AWAKE_PIN, INPUT_PULLDOWN);
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
    saveMsgCounterToFile(++cnt);
    transmitLoRa(cnt);
    if (digitalRead(STAY_AWAKE_PIN))
        stayAwake();
    else
        goToDeepSleep();
}
