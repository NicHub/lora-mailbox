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

With ZSH and mosquitto_pub the limit 10E6 characters

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
