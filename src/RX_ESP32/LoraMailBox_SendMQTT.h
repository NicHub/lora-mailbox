/**
 * LoRa MailBox — MQTT
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 *
 * # Test mosquitto
 * mosquitto_sub -h test.mosquitto.org -t "mailboxtest"
 * mosquitto_pub -h test.mosquitto.org -t "mailboxtest" -m '{"DATE":"'"`date "+%Y-%m-%dT%H:%M:%S+02:00"`"'"}'
 *
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "../common/credentials.h"

class LoraMailBox_SendMQTT
{
private:
    WiFiClient espClient;
    PubSubClient client;
    const char *mqtt_server;
    const int mqtt_port;
    const char *mqtt_topic;

public:
    LoraMailBox_SendMQTT(const char *server = MQTT_SERVER,
                         int port = MQTT_PORT,
                         const char *topic = MQTT_TOPIC)
        : client(espClient), mqtt_server(server), mqtt_port(port), mqtt_topic(topic)
    {
    }

    void reconnect()
    {
        while (!client.connected())
        {
            Serial.print("MQTT connection...");
            String clientId = "ESP32Client-" + String(random(0xffff), HEX);

            if (client.connect(clientId.c_str()))
            {
                Serial.println("connected");
            }
            else
            {
                Serial.print("error, rc=");
                Serial.print(client.state());
                Serial.println(" trying again in 5 seconds");
                delay(5000);
            }
        }
    }

    void begin()
    {
        client.setServer(mqtt_server, mqtt_port);
        reconnect();
    }

    void sendMsg(JsonDocument jsonDoc)
    {
        if (!client.connected())
            reconnect();

        JsonDocument mqttDoc;
        if (!jsonDoc["HEARTBEAT"].isNull())
            mqttDoc["HEARTBEAT"] = jsonDoc["HEARTBEAT"];
        else
        {
            // Reduce string size because knolleary/PubSubClient doesn’t
            // send strings longer than 233 characters on ESP32S3.
            mqttDoc["RSSI (dBm)"] = jsonDoc["RSSI (dBm)"];
            mqttDoc["SNR (dB)"] = jsonDoc["SNR (dB)"];
            mqttDoc["COUNTER"]["VALUE"] = jsonDoc["COUNTER"]["VALUE"];
            mqttDoc["COUNTER"]["ERROR COUNT"] = jsonDoc["COUNTER"]["ERROR COUNT"];
        }
        String mqttString;
        serializeJson(mqttDoc, mqttString);
        client.publish(mqtt_topic, mqttString.c_str());
    }
};
