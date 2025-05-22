#include <Arduino.h>

void setupLoRa()
{
    // # PARAMETERS COMMON TO SX1276 AND SX1262
    float freq = 868.0;
    float bw = 62.5;
    uint8_t sf = 12;
    uint8_t cr = 8;
    uint8_t syncWord = 0x12;
    int8_t power = 14;
    uint16_t preambleLength = 12;

#ifdef NRF52840_XXAA
    rfPort = new SPIClass(
        NRF_SPIM3,
        LoRa_Miso,
        LoRa_Sclk,
        LoRa_Mosi);
    rfPort->begin();

    SPISettings spiSettings;

    radio = new Module(LoRa_Cs, LoRa_Dio1, LoRa_Rst, LoRa_Busy, *rfPort, spiSettings);

    Serial.print("[SX1262] Initializing ...  ");

    // # SX1262 PARAMETERS
    // float freq = 434.0           // freq – Carrier frequency in MHz. Defaults to 434.0 MHz.
    // float bw = 125.0             // bw – LoRa bandwidth in kHz. Defaults to 125.0 kHz.
    // uint8_t sf = 9               // sf – LoRa spreading factor. Defaults to 9.
    // uint8_t cr = 7               // cr – LoRa coding rate denominator. Defaults to 7 (coding rate 4/7).
    // uint8_t syncWord = 0x12      // syncWord – 1-byte LoRa sync word. Defaults to RADIOLIB_SX126X_SYNC_WORD_PRIVATE (0x12).
    // int8_t power = 10            // power – Output power in dBm. Defaults to 10 dBm.
    // uint16_t preambleLength = 8  // preambleLength – LoRa preamble length in symbols. Defaults to 8 symbols.
    // float tcxoVoltage = 1.6      // tcxoVoltage – TCXO reference voltage to be set on DIO3. Defaults to 1.6 V. If you are seeing -706/-707 error codes it likely means you are using non-0 value for module with XTAL. To use XTAL either set this value to 0 or set SX126x::XTAL to true.
    // bool useRegulatorLDO = false // useRegulatorLDO – Whether to use only LDO regulator (true) or DC-DC regulator (false). Defaults to false.

    int state = radio.begin(
        freq,
        bw,
        sf,
        cr,
        syncWord,
        power,
        preambleLength,
        1.6,  // float tcxoVoltage
        false // bool useRegulatorLDO
    );

#elif defined(ESP32)

    Serial.print("[SX1276] Initializing ...  ");

    // # SX1276 PARAMETERS
    // float freq = 434.0           // freq – Carrier frequency in MHz. Allowed values range from 137.0 MHz to 1020.0 MHz.
    // float bw = 125.0             // bw – %LoRa link bandwidth in kHz. Allowed values are 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250 and 500 kHz.
    // uint8_t sf = 9               // sf – %LoRa link spreading factor. Allowed values range from 6 to 12.
    // uint8_t cr = 7               // cr – %LoRa link coding rate denominator. Allowed values range from 5 to 8.
    // uint8_t syncWord = 0x12      // syncWord – %LoRa sync word. Can be used to distinguish different networks. Note that value 0x34 is reserved for LoRaWAN networks.
    // int8_t power = 10            // power – Transmission output power in dBm. Allowed values range from 2 to 17 dBm.
    // uint16_t preambleLength = 8  // preambleLength – Length of %LoRa transmission preamble in symbols. The actual preamble length is 4.25 symbols longer than the set number. Allowed values range from 6 to 65535.
    // uint8_t gain = 0             // gain – Gain of receiver LNA (low-noise amplifier). Can be set to any integer in range 1 to 6 where 1 is the highest gain. Set to 0 to enable automatic gain control (recommended).

    radio = new Module(18, 26, 14, 33); // TTGO  CS, DI0, RST, BUSY

    int state = radio.begin(
        freq,
        bw,
        sf,
        cr,
        syncWord,
        power,
        preambleLength,
        0 // uint8_t gain
    );

#endif

    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.print(("failed, code "));
        Serial.println(state);
    }

    Serial.println(" success");
}

void transmitLora(uint16_t cnt)
{
    delay(1000 - millis() % 1000);
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
    sendMQTT("Hello from ESP32!");
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
        digitalRead(REED_PIN_3) * 100 +
        digitalRead(REED_PIN_2) * 10 +
        digitalRead(REED_PIN_1);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print("\nGPIO : ");
    Serial.printf("%03d", curVal);
    Serial.print("  millis : ");
    Serial.print(millis());
    digitalWrite(LED_BUILTIN, HIGH);
    return curVal;
}

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
#elif OLED_TYPE == 1
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

void setupSerial()
{
    Serial.begin(BAUD_RATE);
    Serial.println(RX);
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
    pinMode(PIR_PIN_0, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);
}

void setupDeepSleep()
{
    if (RXorTX == 0)
        return;
    esp_sleep_enable_ext1_wakeup(MASK, ESP_EXT1_WAKEUP_ANY_HIGH);
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
    Serial.printf(PREFIX "Compilation time %s", COMPILATION_TIME);
    Serial.print(F(PREFIX "Going to deep sleep"));
    Serial.flush();
    delay(1000);
    esp_deep_sleep_start();
}

u_int16_t cntHandler()
{
    while (!LittleFS.begin(true))
    {
        delay(100);
    }
    Serial.println("LittleFS monté avec succès");

    u_int16_t cnt = 0;

    if (LittleFS.exists("/cnt.log"))
    {
        File fichier = LittleFS.open("/cnt.log", "r");
        if (fichier)
        {
            String contenu = fichier.readString();
            fichier.close();
            cnt = contenu.toInt();
            Serial.printf("Valeur lue : %d\n", cnt);
        }
    }
    else
    {
        Serial.println("Le fichier cnt.log n'existe pas, il va être créé");
    }

    cnt++;
    File fichier = LittleFS.open("/cnt.log", "w");
    if (fichier)
    {
        fichier.print(cnt);
        fichier.close();
        Serial.printf("Nouvelle valeur enregistrée : %d\n", cnt);
    }
    else
    {
        Serial.println("Erreur lors de l'ouverture du fichier en écriture");
    }

    return cnt;
}

void setup()
{
    setupGPIOs();
    setupSerial();
    setupLoRa();
    char test = RX;
#if (RXorTX == 0)
    setupWifi();
    setupMQTT();
#else
    u_int16_t cnt = cntHandler();
    transmitLora(cnt);
    debounce(1000);
    setupDeepSleep();
    goToDeepSleep();
    // The Tx module never goes past this point because
    // the ESP restarts upon waking from deep sleep.
#endif
}

void loop()
{
    // For the Rx module only.
    if (!loraEvent)
    {
        yield();
        return;
    }
    loraEvent = false;
    startReceive();
    readLora();
}
