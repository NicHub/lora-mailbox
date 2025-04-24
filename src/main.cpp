#include <Arduino.h>
#include <RadioLib.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#if OLED_TYPE == 0
#include <Adafruit_SSD1306.h>
#endif
#if OLED_TYPE == 1
#include <Adafruit_SH1106.h>
#endif
#include "../fonts/PTMono7pt7b.h"
#include "../fonts/PTMono9pt7b.h"
#include "../fonts/Comic_Sans_MS_Bold13pt7b.h"
#include "send-mail.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 4
#define SCREEN_ADDRESS 0x3C

#if OLED_TYPE == 0
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif
#if OLED_TYPE == 1
Adafruit_SH1106 display(OLED_RESET);
#endif

SX1276 radio = nullptr; // SX1276

#define REED_1 GPIO_NUM_15
#define REED_2 GPIO_NUM_2
#define REED_3 GPIO_NUM_4

// save transmission states between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate transmission or reception state
bool transmitFlag = false;

// flag to indicate that a packet was sent or received
volatile bool loraEvent = false;

// this function is called when a complete packet
// is transmitted or received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif

#define PREFIX "\n[" PROJECT_NAME "] "

void blink(
    unsigned long on_duration_ms = 10,
    unsigned long total_duration_ms = 100,
    unsigned long repeat = 10)
{
    pinMode(LED_BUILTIN, OUTPUT);
    for (size_t i = 0; i < repeat; i++)
    {
        digitalWrite(LED_BUILTIN, LOW);
        delay(on_duration_ms);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(total_duration_ms - on_duration_ms);
    }
}

void setFlag(void)
{
    loraEvent = true;
}

void setupLoRa()
{
    radio = new Module(18, 26, 14, 33); // TTGO  CS, DI0, RST, BUSY

    Serial.print(F(PREFIX "Initializing LoRa ...  "));

    // # SX1276 PARAMETERS
    // float freq = 434.0           // freq – Carrier frequency in MHz. Allowed values range from 137.0 MHz to 1020.0 MHz.
    // float bw = 125.0             // bw – %LoRa link bandwidth in kHz. Allowed values are 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250 and 500 kHz.
    // uint8_t sf = 9               // sf – %LoRa link spreading factor. Allowed values range from 6 to 12.
    // uint8_t cr = 7               // cr – %LoRa link coding rate denominator. Allowed values range from 5 to 8.
    // uint8_t syncWord = 0x12      // syncWord – %LoRa sync word. Can be used to distinguish different networks. Note that value 0x34 is reserved for LoRaWAN networks.
    // int8_t power = 10            // power – Transmission output power in dBm. Allowed values range from 2 to 17 dBm.
    // uint16_t preambleLength = 8  // preambleLength – Length of %LoRa transmission preamble in symbols. The actual preamble length is 4.25 symbols longer than the set number. Allowed values range from 6 to 65535.
    // uint8_t gain = 0             // gain – Gain of receiver LNA (low-noise amplifier). Can be set to any integer in range 1 to 6 where 1 is the highest gain. Set to 0 to enable automatic gain control (recommended).

#define LORA_PARAM 2
#if LORA_PARAM == 0
    int state = radio.begin();
#elif LORA_PARAM == 1 // Default values
    int state = radio.begin(
        434.0, // float freq
        125.0, // float bw
        9,     // uint8_t sf
        7,     // uint8_t cr
        0x12,  // uint8_t syncWord
        10,    // int8_t power
        8,     // uint16_t preambleLength
        0      // uint8_t gain
    );
#elif LORA_PARAM == 2
    int state = radio.begin(
        433.92, // float freq
        31.25,  // float bw
        8,      // uint8_t sf
        8,      // uint8_t cr
        0x12,   // uint8_t syncWord
        17,     // int8_t power
        16,     // uint16_t preambleLength
        0       // uint8_t gain
    );
#endif

    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println(F("success!"));
    }
    else
    {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true)
            yield();
    }

    // set the function that will be called
    // when new packet is received
    radio.setDio0Action(setFlag, RISING);
}

void transmitLora(uint8_t reedVals)
{
    delay(1000 - millis() % 1000);
    static uint16_t cnt = 0;
    String msg =
        ", reedVals: " + String(reedVals) +
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

void readLora()
{
#if (RXorTX == 0)
    String msg;
    int state = radio.readData(msg);
    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf(PREFIX "No packet! state: %d", state);
        return;
    }
    static uint16_t cntMail = 0;
    cntMail++;
    blink();
    Serial.printf(PREFIX "cntMail:\t\t%d", cntMail);
    Serial.printf(PREFIX "Received:\t\t%s", msg.c_str());
    Serial.printf(PREFIX "RSSI:\t\t%.2f dBm", radio.getRSSI());
    Serial.printf(PREFIX "SNR:\t\t%.2f dB", radio.getSNR());

    // OLED
    display.clearDisplay();
    display.display();
    display.setCursor(0, 12);
    display.setFont(&PTMono9pt7b);
    display.print(cntMail);
    display.display();

    sendMail();
#endif
}

void startReceive()
{
    Serial.printf(PREFIX "Start receive");
    radio.startReceive();
}

u_int8_t readReed()
{
    if (RXorTX == 0)
        return 0;
    u_int8_t curVal =
        digitalRead(REED_3) * 100 +
        digitalRead(REED_2) * 10 +
        digitalRead(REED_1);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print("\nGPIO : ");
    Serial.printf("%03d", curVal);
    Serial.print("  millis : ");
    Serial.print(millis());
    digitalWrite(LED_BUILTIN, HIGH);
    return curVal;
}

#if OLED_TYPE == 1
void setupSSH1106()
{
    display.begin(SH1106_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.display();
    if (RXorTX == 1)
        return;
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setFont(&Comic_Sans_MS_Bold13pt7b);
    display.setCursor(2, 20);
    display.println(F("MAILBOX"));
    display.setFont(&PTMono7pt7b);
    display.setCursor(0, 40);
    display.println(COMPILATION_DATE);
    display.print(COMPILATION_TIME);
    display.display();
}
#endif

#if OLED_TYPE == 0
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
    if (RXorTX == 1)
        return;
    display.dim(true);
    display.setRotation(0);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setFont(&Comic_Sans_MS_Bold13pt7b);
    display.setCursor(2, 20);
    display.println(F("MAILBOX"));
    display.setFont(&PTMono7pt7b);
    display.setCursor(0, 40);
    display.println(COMPILATION_DATE);
    display.print(COMPILATION_TIME);
    display.display();
}
#endif

void setupSerial()
{
    Serial.begin(BAUD_RATE);
    if (RXorTX == 1)
        return;
    for (size_t i = 0; i < 3; i++)
    {
        Serial.println(i);
        delay(1000);
    }
}

void setupGPIOs()
{
    pinMode(REED_1, INPUT);
    pinMode(REED_2, INPUT);
    pinMode(REED_3, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);
}

void setupDeepSleep()
{
    if (RXorTX == 0)
        return;
    // uint64_t mask = (1ULL << REED_1) | (1ULL << REED_2) | (1ULL << REED_3);
    uint64_t mask = (1ULL << REED_1);
    esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ANY_HIGH);
}

void debounce(uint32_t wait)
{
    if (RXorTX == 0)
        return;
    delay(wait);
}

void goToDeepSleep()
{
    if (RXorTX == 0)
        return;
    Serial.printf(PREFIX "compilation time %s", COMPILATION_TIME);
    Serial.print(F(PREFIX "Going to deep sleep"));
    Serial.flush();
    delay(1000);
    esp_deep_sleep_start();
}

void setup()
{
    setupGPIOs();
    setupSerial();
#if OLED_TYPE == 0
    setupSSD1306();
#elif OLED_TYPE == 1
    setupSSH1106();
#endif
    setupLoRa();
#if (RXorTX == 0)
    setupWifi();
#endif
    setupDeepSleep();
    uint8_t reedVals = readReed();
    transmitLora(reedVals);
    debounce(1000);
    goToDeepSleep();

    // Le module Tx ne dépasse jamais ce point
    // parce que l’ESP redémarre au réveil du deep sleep.
}

void loop()
{
    // Pour le module Rx uniquement.
    if (!loraEvent)
    {
        yield();
        return;
    }
    loraEvent = false;
    startReceive();
    readLora();
}
