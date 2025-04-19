#include <Arduino.h>
#include <RadioLib.h>

#define SerialMon           Serial
#define PAD_LENGTH 20

const int reedFlapPin = GPIO_NUM_0;
SX1276 radio = nullptr; // SX1276


void setupLoRa()
{
    radio = new Module(18, 26, 14, 33); // TTGO  CS, DI0, RST, BUSY

    SerialMon.print("[SX1276] Initializing ...  ");
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
    int state = radio.begin(
        868.0,                    // float freq              = Fréquence correcte pour l'Europe
        62.5,                     // float bw                = Bande passante réduite
        12,                       // uint8_t sf              = Facteur d'étalement maximal
        8,                        // uint8_t cr              = Taux de codage plus robuste
        SX126X_SYNC_WORD_PRIVATE, // uint8_t syncWord        = SyncWord par défaut
        14,                       // int8_t power            = Puissance maximale autorisée
        12,                       // uint16_t preambleLength = Longueur de préambule augmentée
        1                         // uint8_t gain            // NB this param is different on SX126x => float tcxoVoltage
        // false                  // bool useRegulatorLDO    // NB this param exists on SX126x only
    );

    if (state != ERR_NONE)
    {
        SerialMon.print(("failed, code "));
        SerialMon.println(state);
    }

    SerialMon.println(" success");
}

void loraTransmit()
{
    // you can transmit C-string or Arduino string up to
    // 256 characters long
    // NOTE: loraTransmit() is a blocking method!
    //       See example SX126x_Transmit_Interrupt for details
    //       on non-blocking transmission method.
    // int state = radio.transmit("LilyGO");
    static u_int32_t cnt = 1;
    unsigned long currentTime = cnt;
    char timeString[20];
    sprintf(timeString, "%lu", currentTime);
    // return;
    int state = radio.transmit(timeString);
    SerialMon.printf("  %-*s", PAD_LENGTH, "counter:");
    SerialMon.println(timeString);

    // you can also transmit byte array up to 256 bytes long
    /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x56, 0x78, 0xAB, 0xCD, 0xEF};
    int state = radio.transmit(byteArr, 8);
     */

    SerialMon.printf("  %-*s", PAD_LENGTH, "status:");
    if (state == ERR_NONE)
    {
        // displayCounter(cnt);
        ++cnt;

        // the packet was successfully transmitted
        SerialMon.println(F("success!"));

        // print measured data rate
        SerialMon.printf("  %-*s", PAD_LENGTH, "datarate (bps):");
        SerialMon.println(radio.getDataRate());
    }
    else if (state == ERR_PACKET_TOO_LONG)
    {
        // the supplied packet was longer than 256 bytes
        SerialMon.println(F("too long!"));
    }
    else if (state == ERR_TX_TIMEOUT)
    {
        // timeout occured while transmitting packet
        SerialMon.println(F("tx timeout!"));
    }
    else
    {
        // some other error occurred
        SerialMon.print(F("failed, code "));
        SerialMon.println(state);
    }
}

void setup()
{
    setupLoRa();
    SerialMon.begin(BAUD_RATE);
    SerialMon.flush();
    pinMode(reedFlapPin, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    loraTransmit();
    delay(1000); // debounce
    SerialMon.println("DONE");
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    esp_deep_sleep_start();
}

void loop()
{
    yield();
}
