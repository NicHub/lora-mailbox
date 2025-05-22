#include <Arduino.h>
#include "common/common.h"
#include "common/LoraMailBox_Settings.h"

void goToDeepSleep()
{
    Serial.printf(PREFIX "Compilation date %s", COMPILATION_DATE);
    Serial.printf(PREFIX "Compilation time %s", COMPILATION_TIME);
    Serial.print(F(PREFIX "Going to deep sleep"));
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

void transmitLora(uint16_t cnt)
{
    String msg =
        " cnt: " + String(++cnt) +
        ", millis: " + String(millis()) +
        ", HelloFrom: " + PROJECT_NAME +
        ", CompilationTime: " + __TIME__;
    Serial.printf(PREFIX "Sending\t\t%s", msg.c_str());
    int state = radio.startTransmit(msg);
    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf(PREFIX "failed, code %d", state);
        return;
    }
    Serial.print(F(PREFIX "transmission finished!"));
}

uint16_t cntHandler()
{
    while (!LittleFS.begin(true))
    {
        delay(100);
    }
    Serial.print(F(PREFIX "LittleFS mounted successfully"));

    uint16_t cnt = 0;

#define CNT_LOG_FILENAME "/cnt.log"

    if (LittleFS.exists(CNT_LOG_FILENAME))
    {
        File fichier = LittleFS.open(CNT_LOG_FILENAME, "r");
        if (fichier)
        {
            String contenu = fichier.readString();
            fichier.close();
            cnt = contenu.toInt();
            Serial.printf(PREFIX "cnt = %d", cnt);
        }
    }
    else
    {
        Serial.print(F(PREFIX "The file " CNT_LOG_FILENAME " does not exist, it will be created"));
    }

    cnt++;
    File fichier = LittleFS.open(CNT_LOG_FILENAME, "w");
    if (fichier)
    {
        fichier.print(cnt);
        fichier.close();
        Serial.printf(PREFIX "New value saved: %d", cnt);
    }
    else
    {
        Serial.print(F(PREFIX "Error opening file for writing"));
    }

    return cnt;
}

void setupSerial()
{
    Serial.begin(BAUD_RATE);
    for (size_t i = 0; i < 1; i++)
    {
        Serial.println(i);
        delay(1000);
    }
}

void setupGPIOs()
{
    // Beware that on XIAO ESP32S3,
    // LORA_USER_BUTTON and BUILTIN_LED
    // are both connected on GPIO21!
    // So if you set BUILTIN_LED pinMode to
    // OUTPUT and you set BUILTIN_LED to HIGH
    // and you press LORA_USER_BUTTON,
    // you create a short-circuit between
    // GPIIO21 and GND.
    pinMode(LORA_USER_BUTTON, INPUT);
    pinMode(PIR_PIN_0, INPUT);
    pinMode(LORA_GREEN_LED, OUTPUT);
}

void setup()
{
    setupGPIOs();
    setupSerial();
    setupLoRa();
}

void loop()
{
    yield();
    uint16_t cnt = cntHandler();
    transmitLora(cnt);
    debounce(5000);
    // Set DEEP_SLEEP to false to perform signal quality tests.
#define DEEP_SLEEP true
#if DEEP_SLEEP
    setupDeepSleep();
    goToDeepSleep();
#endif
    // If the module goes to deep sleep, this is never executed
    // because the ESP restarts upon waking up.
    blink();
}
