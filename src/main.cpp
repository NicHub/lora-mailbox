#include <Arduino.h>

const int reedFlapPin = GPIO_NUM_0;

#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 4
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void clearSSD1306()
{
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        while (true)
            yield();
    }
    display.clearDisplay();
    display.display();
}

void setup()
{
    pinMode(reedFlapPin, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);
#if false
    clearSSD1306();
#endif

    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    esp_deep_sleep_start();
}

void loop()
{
    yield();
}
