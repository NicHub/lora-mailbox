#include <Arduino.h>
#include <RadioLib.h>


#include <SPI.h>
#include <Wire.h>
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
    // carrier frequency:           868.0 MHz
    // bandwidth:                   125.0 kHz
    // spreading factor:            9
    // coding rate:                 7
    // sync word:                   0x12 (private network)
    // output power:                14 dBm
    // current limit:               60 mA
    // preamble length:             8 symbols
    // TCXO voltage:                1.6 V (set to 0 to not use TCXO)
    // regulator:                   DC-DC (set to true to use LDO)
    // CRC:                         enabled
    // int state = radio.begin(
    //     868.0, // float freq              = Fréquence correcte pour l'Europe
    //     62.5,  // float bw                = Bande passante réduite
    //     12,    // uint8_t sf              = Facteur d'étalement maximal
    //     8,     // uint8_t cr              = Taux de codage plus robuste
    //     0x12,  // uint8_t syncWord        = SyncWord par défaut
    //     14,    // int8_t power            = Puissance maximale autorisée
    //     12,    // uint16_t preambleLength = Longueur de préambule augmentée
    //     1      // uint8_t gain            // NB this param is different on SX126x => float tcxoVoltage
    //            // false                  // bool useRegulatorLDO    // NB this param exists on SX126x only
    // );
    int state = radio.begin();

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
    display.setTextSize(1);
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
    setupSSD1306();
    setupLoRa();
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
