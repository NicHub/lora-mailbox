#include <Arduino.h>
#include <axp20x.h>
AXP20X_Class axp;

const int reedFlapPin = GPIO_NUM_0;

void setup()
{
    Wire.begin(21, 22);
    // axp.setChgLEDMode(AXP20X_LED_LOW_LEVEL);

    axp.setChgLEDMode(AXP20X_LED_OFF);       // LED off
    axp.setChgLEDMode(AXP20X_LED_BLINK_1HZ); // 1blink/sec, low rate
    axp.setChgLEDMode(AXP20X_LED_BLINK_4HZ); // 4blink/sec, high rate
    axp.setChgLEDMode(AXP20X_LED_LOW_LEVEL); // LED full on

    pinMode(reedFlapPin, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    // delay(100);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    esp_deep_sleep_start();
}

void loop()
{
    yield();
}
