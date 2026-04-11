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

```

mosquitto_sub -h mqtt.ouilogique.ch -p 8883 -u guest -P guest123 -t "mbx_nj/#" | jq .
mosquitto_sub -h mqtt.ouilogique.ch -p 8883 -u guest -P guest123 -t "mbx_nj/#" -v
mosquitto_sub -h mqtt.ouilogique.ch -p 8883 -u guest -P guest123 -t "mbx_nj/+/heartbeat_rx" -v
mosquitto_sub -h mqtt.ouilogique.ch -p 8883 -u guest -P guest123 -t "mbx_nj/98:3D:AE:ED:0E:E4/heartbeat_rx" -v

mosquitto_sub -h mqtt.ouilogique.ch -p 8883 -u guest -P guest123 -t "mbx_rz/#" | jq .
mosquitto_sub -h mqtt.ouilogique.ch -p 8883 -u guest -P guest123 -t "mbx_rz/#" -v
mosquitto_sub -h mqtt.ouilogique.ch -p 8883 -u guest -P guest123 -t "mbx_rz/+/heartbeat_rx" -v
mosquitto_sub -h mqtt.ouilogique.ch -p 8883 -u guest -P guest123 -t "mbx_rz/B8:F8:62:F8:B0:E8/heartbeat_rx" -v

```

-   ESP32 Deep Sleep Current: What the Datasheet Says vs What You'll Actually Measure
    -   https://hubble.com/community/guides/esp32-deep-sleep-current-what-the-datasheet-says-vs-what-you-ll-actually-measure/#three-fixes-that-dont-require-a-custom-pcb

-   TTN (The Things Network) Platform Connection & Usage
    -   https://docs.m5stack.com/en/guide/lora/lorawan/ttn

-   ESP32 Guide 2026
    -   https://www.youtube.com/watch?v=CfIjInYch7U
    -   https://dronebotworkshop.com/esp32-2026/
    -   https://dronebotworkshop.com/esp32-2026/#ESP32_Variant_Comparison
    -   https://dronebotworkshop.com/esp32-2026/#General_Purpose_Experimenter_Boards




------



```jsonl
{
  "last_commit_id": "3339c59 (0 uncommitted files)",
  "BOARD_ID_HEX": "FBBF6F978A698846",
  "WAKEUP": "HEARTBEAT_TX",
  "VGPIO": 356,
  "VBAT_MV": 3791,
  "VBAT_PERCENT": 38,
  "VBAT_GLYPH": "▄",
  "VBAT_STATUS": "OK",
  "CURRENT_TIME": "2026-04-11 16:41:07",
  "COMPILATION_DATE": "2026-04-01",
  "COMPILATION_TIME": "14:58:26",
  "RSSI_DBM": -120,
  "SNR_DB": -1.5,
  "WEB_UI_URL": "http://192.168.1.106",
  "WS_CLIENT_COUNT": 0,
  "STATE": 0,
  "JSON_STRING": "{\"board_id_hex\":\"FBBF6F978A698846\",\"cnt\":1491,\"volt_gpio\":356,\"wakeup\":\"HEARTBEAT_TX\",\"last_commit_id\":\"3339c59 (0 uncommitted files)\"}",
  "DEBUG": true,
  "COUNTER": {
    "VALUE": 1491,
    "PREVIOUS_VALUE": 1490,
    "ERROR_COUNT": 7,
    "STATUS": "OK"
  }
}
{
  "HEARTBEAT_RX": "2026-04-11 16:41:08",
  "BOARD_ID_HEX": "B8:F8:62:F8:B0:E8",
  "WEB_UI_URL": "http://192.168.1.106",
  "COMPILATION_DATE": "2026-04-01",
  "COMPILATION_TIME": "14:58:26"
}
```
