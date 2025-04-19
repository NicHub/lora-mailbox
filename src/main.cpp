#include <Arduino.h>

const int inputPin = 0;

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <../fonts/PTMono7pt7b.h>
#include <../fonts/PTMono9pt7b.h>
#include "../fonts/Comic_Sans_MS_Bold13pt7b.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 4
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setupSSD1306()
{
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        Serial.println(F("SSD1306 allocation failed"));
        while (true)
            yield();
    }
    display.clearDisplay();
    display.display();
    display.dim(true);
    display.setRotation(0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setFont(&Comic_Sans_MS_Bold13pt7b);
    display.setCursor(2, 20);
    display.println(F("GPIO"));
    display.setFont(&PTMono7pt7b);
    display.setCursor(0, 40);
    display.println(COMPILATION_DATE);
    display.print(COMPILATION_TIME);
    display.display();
}

void setup()
{
    pinMode(inputPin, INPUT_PULLUP);
    Serial.begin(BAUD_RATE);
    setupSSD1306();
}

void loop()
{
    yield();
    // delay(500);

    // int state = digitalRead(inputPin);
    // Serial.print("Pin state: ");
    // Serial.println(state);
    // if (state)
    //     return;
    // display.clearDisplay();
    // display.print(F("STATE "));
    // display.println(state);
    // display.display();
}
