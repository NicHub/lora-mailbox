/**
 * LoRa MailBox — MQTT
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <Arduino.h>
#include <PsychicMqttClient.h>
#include <WiFi.h>
#include "../common/user_settings.h"

class LoraMailBox_SendMQTT
{
private:
#if MQTT_USE_TLS
    static constexpr const char *MQTT_ROOT_CA = R"PEM(-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)PEM";
#endif
    PsychicMqttClient client;
    const char *mqtt_server;
    const int mqtt_port;
    const char *mqtt_topic;
    String mqtt_uri;
    bool mqttStarted = false;
    bool mqttConnected = false;
    String pendingPayload;

    bool ensureWiFiConnected()
    {
        if (WiFi.status() == WL_CONNECTED)
            return true;

        Serial.printf("WiFi not connected, status=%d\n", WiFi.status());
        return false;
    }

    bool publishPayload(const String &payload)
    {
        const int publishResult = client.publish(mqtt_topic, 0, false, payload.c_str());
        if (publishResult < 0)
        {
            Serial.printf("MQTT publish failed: connected=%d payload=%u topic_len=%u uri=%s\n",
                          mqttConnected,
                          static_cast<unsigned>(payload.length()),
                          static_cast<unsigned>(strlen(mqtt_topic)),
                          mqtt_uri.c_str());
            return false;
        }

        // Serial.printf("MQTT publish ok: msgId=%d payload=%u\n",
        //               publishResult,
        //               static_cast<unsigned>(payload.length()));
        return true;
    }

public:
    LoraMailBox_SendMQTT(const char *server = MQTT_SERVER,
                         int port = MQTT_PORT,
                         const char *topic = MQTT_TOPIC)
        : mqtt_server(server), mqtt_port(port), mqtt_topic(topic)
    {
#if MQTT_USE_TLS
        mqtt_uri = String("mqtts://") + mqtt_server + ":" + mqtt_port;
#else
        mqtt_uri = String("mqtt://") + mqtt_server + ":" + mqtt_port;
#endif
    }

    void reconnect()
    {
        if (!ensureWiFiConnected())
            return;

        if (mqttStarted || mqttConnected)
            return;

        client.connect();
        mqttStarted = true;
    }

    void begin()
    {
        client.setServer(mqtt_uri.c_str());
#if MQTT_USE_TLS
        client.setCACert(MQTT_ROOT_CA);
#endif
        if (strlen(MQTT_USERNAME) > 0)
            client.setCredentials(MQTT_USERNAME, MQTT_PASSWORD);
        client.setBufferSize(1024);
        client.setAutoReconnect(true);
        client.onConnect([this](bool sessionPresent)
                         {
                             mqttConnected = true;
                             mqttStarted = true;
                             Serial.printf("MQTT connected, sessionPresent=%d\n", sessionPresent);

                             if (!pendingPayload.isEmpty())
                             {
                                 String payload = pendingPayload;
                                 pendingPayload = "";
                                 publishPayload(payload);
                             }
                         });
        client.onDisconnect([this](bool sessionPresent)
                            {
                                mqttConnected = false;
                                mqttStarted = false;
                                Serial.printf("MQTT disconnected, sessionPresent=%d\n", sessionPresent);
                            });
        client.onError([this](esp_mqtt_error_codes_t error)
                       {
                           mqttConnected = false;
                           Serial.printf("MQTT error: type=%d connect_return=%d esp_tls_last=0x%x tls_stack=0x%x sock_errno=%d\n",
                                         error.error_type,
                                         error.connect_return_code,
                                         error.esp_tls_last_esp_err,
                                         error.esp_tls_stack_err,
                                         error.esp_transport_sock_errno);
                       });
        reconnect();
    }

    void sendMsg(JsonDocument jsonDoc)
    {
        if (!ensureWiFiConnected())
            return;

        String mqttString;
        serializeJson(jsonDoc, mqttString);

        reconnect();
        if (!mqttConnected)
        {
            pendingPayload = mqttString;
            Serial.printf("MQTT publish deferred: connected=%d payload=%u uri=%s\n",
                          mqttConnected,
                          static_cast<unsigned>(mqttString.length()),
                          mqtt_uri.c_str());
            return;
        }

        publishPayload(mqttString);
    }
};
