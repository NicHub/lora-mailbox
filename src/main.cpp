#include <Arduino.h>

const int reedFlapPin = GPIO_NUM_0;

void setup()
{
    pinMode(reedFlapPin, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    esp_deep_sleep_start();
}

void loop()
{
    yield();
}
