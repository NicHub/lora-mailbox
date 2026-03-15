# Scratchpad LoRa Mailbox

https://wiki.seeedstudio.com/XIAO_BLE/

Hi,

In your schematic, P0.17_CHG drives the RED_CHG LED but is also tied to the PRETERM pin of the BQ25100.
According to the datasheet, PRETERM should only be connected to a resistor to GND to set pre-charge and termination currents.

Could you clarify why P0.17_CHG is connected in both places?

Thanks!

Set P0.14 to output Sink only to enable BAT voltage read

## Pinout

| schematics¹    | variant.h                 | pin mode | state                                          |
| -------------- | ------------------------- | -------- | ---------------------------------------------- |
| P0.13_HICHG    | PIN_CHARGING_CURRENT (22) | OUTPUT   | LOW: High charging current (100 mA)            |
|                |                           |          | HIGH: Low charging current (50 mA)             |
|                |                           |          |                                                |
| P0.14_READ_BAT | VBAT_ENABLE (14)          | OUTPUT   | Must be set to HIGH when charging              |
|                |                           | INPUT    | Pin mode INPUT ensure lowest current           |
|                |                           |          | consumption when battery reading is not needed |
|                |                           |          |                                                |
| P0.17_CHG      | ?                         | INPUT    | LOW: charging in progress                      |
|                |                           |          | HIGH: not charging or fully charged            |
|                |                           |          |                                                |
| P0.31_AIN7_BAT | PIN_VBAT (32)             | INPUT    | Analog read                                    |

¹ https://files.seeedstudio.com/wiki/XIAO-BLE/Seeed-Studio-XIAO-nRF52840-Sense-v1.1.pdf

BQ25100 https://www.ti.com/lit/ds/symlink/bq25100a.pdf

---

## MQTT charcter count limit

-   [knolleary/PubSubClient] character count per message is limited to 233.
-   [PsychicMqttClient] character count per message is limited by hardware.

[knolleary/PubSubClient]: https://registry.platformio.org/libraries/knolleary/PubSubClient
[PsychicMqttClient]: https://github.com/theelims/PsychicMqttClient.git

```cpp
    // Check the maximum number of character that can be sent with knolleary/PubSubClient on ESP32S3
    // The answer is 233.
    Serial.println("Starting string length test to test.mosquitto.org...");
    WiFiClient mqttTestClient;
    PubSubClient mqttTest(mqttTestClient);
    mqttTest.setServer("test.mosquitto.org", 1883);

    // Connect to the test broker
    if (mqttTest.connect("ESP32LoRaMailboxClient")) {
        // Loop from 0 to 999
        for (int i = 0; i < 1000; i++) {
            char message[i];
            String fmt = "%0" + String(i) + "d";
            sprintf(message, fmt.c_str(), i);

            // Publish the string to the stringlengthtest topic
            mqttTest.publish("stringlengthtest", message);

            Serial.printf("Published to test.mosquitto.org: %s\n", message);
            delay(100); // Small delay to avoid overwhelming the broker
        }
        mqttTest.disconnect();
        Serial.println("String length test completed.");
    } else {
        Serial.println("Failed to connect to test.mosquitto.org");
    }
```

With ZSH and mosquitto_pub the limit is 10E6 characters

zsh: argument list too long: mosquitto_pub

```zsh
mosquitto_sub -h test.mosquitto.org -t "stringlengthtest"
```

```zsh
for ((i=1E6; i <= 10E6; i+=1E6)); do
    message=$(printf "%0${i}d" $i)
    echo "i=$i: $message"
    mosquitto_pub -h test.mosquitto.org -t stringlengthtest -m "$message"
done
```

```bash
mosquitto_sub -h test.mosquitto.org -t "mailboxtest"

pio device monitor -e seeed_xiao_esp32s3-rx
```

```

mosquitto_sub -h mqtt.ouilogique.ch -p 8883 -u guest -P guest123 -t "mbx_nj/#" -v
mosquitto_sub -h mqtt.ouilogique.ch -p 8883 -u guest -P guest123 -t "mbx_rz/#" -v

```

# IoT BMS Requirements

Goal: choose a safe and reliable battery solution for small IoT devices, with long battery life and easy integration.

**Must-have features**

-   Overcharge protection
-   Deep discharge protection
-   Short circuit protection
-   Reverse polarity protection
-   Minimal self-consumption
-   Cost compatible with Arduino
-   Easily available

**Nice-to-have features**

-   Outdoor use compliant
-   Solar charging support
-   USB charging support
-   Ability to power the device while charging


## Vérifier platformio.ini

```bash
pio project config
```

# IDs

rolf
18140340520724564038
FBBF6F978A698846
nico
5122412905211900040
47167765C8083088

# TODO

...

# LoRa settings

```cpp
#define LORA_SETTINGS 1

#if LORA_SETTINGS == 0
// Default parameters defined in SX1262.h.
#define LORA_FREQ 434.0 // Seeed XIA SX1262 range is 862 - 930 MHz
#define LORA_BW 125.0
#define LORA_SF 9
#define LORA_CR 7
#define LORA_SYNCWORD RADIOLIB_SX126X_SYNC_WORD_PRIVATE
#define LORA_POWER 10
#define LORA_PREAMBLELENGTH 8
#define LORA_TCXOVOLTAGE 1.6
#define LORA_USEREGULATORLDO false

#elif LORA_SETTINGS == 1
#define LORA_FREQ 868.0
#define LORA_BW 62.5
#define LORA_SF 12
#define LORA_CR 8
#define LORA_SYNCWORD RADIOLIB_SX126X_SYNC_WORD_PRIVATE
#define LORA_POWER 14
#define LORA_PREAMBLELENGTH 12
#define LORA_TCXOVOLTAGE 1.6
#define LORA_USEREGULATORLDO false
#endif
```
