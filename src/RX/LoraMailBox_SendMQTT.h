/**
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 *
 * # Test mosquitto
 * mosquitto_sub -h test.mosquitto.org -t "mailboxtest"
 * mosquitto_pub -h test.mosquitto.org -t "mailboxtest" -m "Hello world"
 *
 */

#pragma once

#include <Arduino.h>

#include <WiFi.h>
#include <PubSubClient.h>
#include "credentials.h"

// const char *mqtt_server = "test.mosquitto.org";
// const int mqtt_port = 1883;
// const char *mqtt_topic = "mailboxnj";

WiFiClient espClient;
PubSubClient client(espClient);

void reconnectMQTT()
{
    while (!client.connected())
    {
        Serial.print("Connexion MQTT...");
        String clientId = "ESP32Client-" + String(random(0xffff), HEX);

        if (client.connect(clientId.c_str()))
        {
            Serial.println("connecté");
        }
        else
        {
            Serial.print("échec, rc=");
            Serial.print(client.state());
            Serial.println(" nouvel essai dans 5 secondes");
            delay(5000);
        }
    }
}

void setupMQTT()
{
    client.setServer(mqtt_server, mqtt_port);
    reconnectMQTT();
}

void sendMQTT(const char *message)
{
    if (!client.connected())
    {
        reconnectMQTT();
    }
    client.publish(mqtt_topic, message);
    Serial.println("Message envoyé: " + String(message));
}

// void setupWifi()
// {
//     WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//     while (WiFi.status() != WL_CONNECTED)
//     {
//         delay(500);
//         Serial.print(".");
//     }
//     Serial.print("\nWiFi connecté\nhttp://");
//     Serial.println(WiFi.localIP());
// }

// void setup()
// {
//     Serial.begin(115200);
//     setupWifi();
//     setupMQTT();
// }

// void loop()
// {
//     // yield();
//     // return;
//     client.loop();
//     sendMQTTMessage("Hello from ESP32!");
//     delay(5000); // Attend 5 secondes avant le prochain envoi
// }