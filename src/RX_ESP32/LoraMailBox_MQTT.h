/**
 * LoRa MailBox — MQTT
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PsychicMqttClient.h>
#include <WiFi.h>
#include "user_settings/user_settings.h"

class LoraMailBox_MQTT
{
private:
#if MQTT_USE_TLS
    static constexpr const char *MQTT_ROOT_CA = MQTT_ROOT_CA_PEM;
#endif
    PsychicMqttClient client;
    const char *mqtt_server;
    const int mqtt_port;
    const char *mqtt_default_topic;
    String mqtt_uri;
    bool mqttStarted = false;
    bool mqttConnected = false;
    String pendingPayload;
    String pendingTopics;

    static constexpr size_t MAX_TOPICS_PER_MESSAGE = 8;

    /**
     * @brief Return true when Wi-Fi is connected.
     */
    bool ensureWiFiConnected()
    {
        if (WiFi.status() == WL_CONNECTED)
            return true;

        Serial.printf("WiFi not connected, status=%d\n", WiFi.status());
        return false;
    }

    /**
     * @brief Add one topic to the output list if it is non-empty and not already present.
     * @param topic Candidate topic.
     * @param topics Output array of topics.
     * @param topicCount Current topic count.
     * @param maxTopics Maximum number of topics accepted.
     */
    void addTopicIfUnique(const char *topic, const char **topics, size_t &topicCount, size_t maxTopics) const
    {
        if (topic == nullptr || topic[0] == '\0')
            return;
        for (size_t i = 0; i < topicCount; ++i)
        {
            if (strcmp(topics[i], topic) == 0)
                return;
        }
        if (topicCount >= maxTopics)
            return;
        topics[topicCount++] = topic;
    }

    /**
     * @brief Add one topic string to the output list if it is non-empty and not already present.
     * @param topic Candidate topic.
     * @param topics Output array of topic strings.
     * @param topicCount Current topic count.
     * @param maxTopics Maximum number of topics accepted.
     */
    void addTopicStringIfUnique(const String &topic, String *topics, size_t &topicCount, size_t maxTopics) const
    {
        if (topic.isEmpty())
            return;
        for (size_t i = 0; i < topicCount; ++i)
        {
            if (topics[i] == topic)
                return;
        }
        if (topicCount >= maxTopics)
            return;
        topics[topicCount++] = topic;
    }

    /**
     * @brief Resolve all MQTT topics matching the message categories.
     * @param jsonDoc Message payload.
     * @param topics Output array of topics.
     * @param maxTopics Maximum number of topics accepted.
     * @return Number of topics selected.
     */
    size_t resolveTopics(const JsonDocument &jsonDoc, const char **topics, size_t maxTopics) const
    {
        size_t topicCount = 0;
        const char *wakeup = jsonDoc["WAKEUP"] | "";
        if (wakeup[0] == '\0')
            wakeup = jsonDoc["wakeup"] | "";
        uint16_t batteryMv = jsonDoc["VGPIO"] | 0;
        if (batteryMv == 0)
            batteryMv = jsonDoc["volt_gpio"] | 0;

#if defined(MQTT_CUSTOM_TOPIC_RESOLVER)
        const char *customTopic = MQTT_CUSTOM_TOPIC_RESOLVER(jsonDoc, wakeup, batteryMv);
        addTopicIfUnique(customTopic, topics, topicCount, maxTopics);
#endif

#if defined(MQTT_TOPIC_LOW_BATTERY)
        const char *batteryStatus = jsonDoc["VBAT_STATUS"] | "";
        if (strcmp(batteryStatus, "LOW") == 0)
            addTopicIfUnique(MQTT_TOPIC_LOW_BATTERY, topics, topicCount, maxTopics);
#endif

#if defined(MQTT_TOPIC_HEARTBEAT_RX)
        if (!jsonDoc["HEARTBEAT_RX"].isNull())
            addTopicIfUnique(MQTT_TOPIC_HEARTBEAT_RX, topics, topicCount, maxTopics);
#endif

#if defined(MQTT_TOPIC_WAKEUP_PIN_HIGH)
        if (strcmp(wakeup, "WAKEUP_PIN_HIGH") == 0)
            addTopicIfUnique(MQTT_TOPIC_WAKEUP_PIN_HIGH, topics, topicCount, maxTopics);
#endif

#if defined(MQTT_TOPIC_HEARTBEAT_TX)
        if (strcmp(wakeup, "HEARTBEAT_TX") == 0)
            addTopicIfUnique(MQTT_TOPIC_HEARTBEAT_TX, topics, topicCount, maxTopics);
#endif

#if defined(MQTT_TOPIC_BOOT)
        if (strcmp(wakeup, "BOOT") == 0)
            addTopicIfUnique(MQTT_TOPIC_BOOT, topics, topicCount, maxTopics);
#endif
        if (topicCount == 0)
            addTopicIfUnique(mqtt_default_topic, topics, topicCount, maxTopics);
        return topicCount;
    }

    /**
     * @brief Publish one payload on a specific MQTT topic.
     * @param payload Serialized JSON payload.
     * @param topic Destination topic.
     * @return true when publish request succeeded.
     */
    bool publishPayload(const String &payload, const char *topic)
    {
        const int publishResult = client.publish(topic, 0, false, payload.c_str());
        if (publishResult < 0)
        {
            Serial.printf("MQTT publish failed: connected=%d payload=%u topic=%s uri=%s\n",
                          mqttConnected,
                          static_cast<unsigned>(payload.length()),
                          topic,
                          mqtt_uri.c_str());
            return false;
        }

        return true;
    }

    /**
     * @brief Inject `board_id_hex` as second topic level when available.
     * @param topic Base topic (for example `mbx_nj/low_battery`).
     * @param boardIdHex Board identifier extracted from payload.
     * @return Topic with board id as second level.
     */
    String topicWithBoardId(const char *topic, const char *boardIdHex) const
    {
        if (topic == nullptr)
            return "";
        if (boardIdHex == nullptr || boardIdHex[0] == '\0')
            return String(topic);

        const char *firstSlash = strchr(topic, '/');
        if (firstSlash == nullptr)
            return String(topic) + "/" + boardIdHex;

        String prefix(topic, static_cast<unsigned>(firstSlash - topic));
        const char *suffix = firstSlash + 1;
        if (suffix[0] == '\0')
            return prefix + "/" + boardIdHex;
        return prefix + "/" + boardIdHex + "/" + suffix;
    }

    /**
     * @brief Publish one payload to all topics from a line-separated topic list.
     * @param payload Serialized JSON payload.
     * @param topicList Newline-separated topic list.
     * @return true when all publish operations succeeded.
     */
    bool publishPayloadToTopicList(const String &payload, const String &topicList)
    {
        if (topicList.isEmpty())
            return publishPayload(payload, mqtt_default_topic);

        bool ok = true;
        int start = 0;
        while (start < static_cast<int>(topicList.length()))
        {
            int end = topicList.indexOf('\n', start);
            if (end < 0)
                end = topicList.length();
            String topic = topicList.substring(start, end);
            if (!topic.isEmpty())
            {
                if (!publishPayload(payload, topic.c_str()))
                    ok = false;
            }
            start = end + 1;
        }
        return ok;
    }

public:
    LoraMailBox_MQTT(const char *server = MQTT_SERVER,
                         int port = MQTT_PORT,
                         const char *topic = MQTT_TOPIC)
        : mqtt_server(server), mqtt_port(port), mqtt_default_topic(topic)
    {
#if MQTT_USE_TLS
        mqtt_uri = String("mqtts://") + mqtt_server + ":" + mqtt_port;
#else
        mqtt_uri = String("mqtt://") + mqtt_server + ":" + mqtt_port;
#endif
    }

    void reconnect()
    {
#if !MQTT_ENABLED
        return;
#endif
        if (!ensureWiFiConnected())
            return;

        if (mqttStarted || mqttConnected)
            return;

        client.connect();
        mqttStarted = true;
    }

    void begin()
    {
#if !MQTT_ENABLED
        return;
#endif
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
                                 String topics = pendingTopics;
                                 pendingPayload = "";
                                 pendingTopics = "";
                                 publishPayloadToTopicList(payload, topics);
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
#if !MQTT_ENABLED
        (void)jsonDoc;
        return;
#endif
        if (!ensureWiFiConnected())
            return;

        String mqttString;
        serializeJson(jsonDoc, mqttString);
        const char *topics[MAX_TOPICS_PER_MESSAGE];
        size_t topicCount = resolveTopics(jsonDoc, topics, MAX_TOPICS_PER_MESSAGE);
        const char *boardIdHex = jsonDoc["BOARD_ID_HEX"] | "";
        if (boardIdHex[0] == '\0')
            boardIdHex = jsonDoc["board_id_hex"] | "";

        String finalTopics[MAX_TOPICS_PER_MESSAGE];
        size_t finalTopicCount = 0;
        String topicList;
        for (size_t i = 0; i < topicCount; ++i)
        {
            String routedTopic = topicWithBoardId(topics[i], boardIdHex);
            addTopicStringIfUnique(routedTopic, finalTopics, finalTopicCount, MAX_TOPICS_PER_MESSAGE);
        }

        for (size_t i = 0; i < finalTopicCount; ++i)
        {
            if (i > 0)
                topicList += '\n';
            topicList += finalTopics[i];
        }

        reconnect();
        if (!mqttConnected)
        {
            pendingPayload = mqttString;
            pendingTopics = topicList;
            Serial.printf("MQTT publish deferred: connected=%d payload=%u topic_count=%u uri=%s\n",
                          mqttConnected,
                          static_cast<unsigned>(mqttString.length()),
                          static_cast<unsigned>(finalTopicCount),
                          mqtt_uri.c_str());
            return;
        }

        publishPayloadToTopicList(mqttString, topicList);
    }
};
