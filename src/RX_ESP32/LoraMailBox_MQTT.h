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

class LoraMailboxMqtt
{
private:
    static constexpr const char *MQTT_ROOT_CA = settings::mqtt::root_ca_pem;

    PsychicMqttClient client;
    const char *mqtt_server;
    const int mqtt_port;
    const char *mqtt_default_topic;
    String mqtt_uri;
    bool mqtt_started = false;
    bool mqtt_connected = false;
    String pending_payload;
    String pending_topics;

    static constexpr size_t MAX_TOPICS_PER_MESSAGE = 8;

    bool ensureWiFiConnected()
    {
        if (WiFi.status() == WL_CONNECTED)
            return true;

        Serial.printf("WiFi not connected, status=%d\n", WiFi.status());
        return false;
    }

    void addTopicIfUnique(const char *topic, const char **topics, size_t &topic_count, size_t max_topics) const
    {
        if (topic == nullptr || topic[0] == '\0')
            return;
        for (size_t i = 0; i < topic_count; ++i)
        {
            if (strcmp(topics[i], topic) == 0)
                return;
        }
        if (topic_count >= max_topics)
            return;
        topics[topic_count++] = topic;
    }

    void addTopicStringIfUnique(const String &topic, String *topics, size_t &topic_count, size_t max_topics) const
    {
        if (topic.isEmpty())
            return;
        for (size_t i = 0; i < topic_count; ++i)
        {
            if (topics[i] == topic)
                return;
        }
        if (topic_count >= max_topics)
            return;
        topics[topic_count++] = topic;
    }

    size_t resolveTopics(const JsonDocument &json_doc, const char **topics, size_t max_topics) const
    {
        size_t topic_count = 0;
        const char *rx_trigger = json_doc["RX"]["RX_TRIGGER"] | "";
        const char *tx_trigger = json_doc["TX"]["TX_TRIGGER"] | "";
        if (strcmp(rx_trigger, "HEARTBEAT_RX") == 0)
            addTopicIfUnique(settings::mqtt::topic_heartbeat_rx, topics, topic_count, max_topics);

        if (strcmp(tx_trigger, "WAKEUP_PIN_HIGH") == 0)
            addTopicIfUnique(settings::mqtt::topic_got_mail, topics, topic_count, max_topics);

        if (strcmp(tx_trigger, "HEARTBEAT_TX") == 0)
            addTopicIfUnique(settings::mqtt::topic_heartbeat_tx, topics, topic_count, max_topics);

        if (strcmp(tx_trigger, "BOOT") == 0)
            addTopicIfUnique(settings::mqtt::topic_wake_boot, topics, topic_count, max_topics);

        if (topic_count == 0)
            addTopicIfUnique(mqtt_default_topic, topics, topic_count, max_topics);
        return topic_count;
    }

    bool publishPayload(const String &payload, const char *topic)
    {
        const int publish_result = client.publish(topic, 0, false, payload.c_str());
        if (publish_result < 0)
        {
            Serial.printf("MQTT publish failed: connected=%d payload=%u topic=%s uri=%s\n",
                          mqtt_connected,
                          static_cast<unsigned>(payload.length()),
                          topic,
                          mqtt_uri.c_str());
            return false;
        }

        return true;
    }

    String topicWithBoardId(const char *topic, const char *board_id_hex) const
    {
        if (topic == nullptr)
            return "";
        if (board_id_hex == nullptr || board_id_hex[0] == '\0')
            return String(topic);

        const char *first_slash = strchr(topic, '/');
        if (first_slash == nullptr)
            return String(topic) + "/" + board_id_hex;

        String prefix(topic, static_cast<unsigned>(first_slash - topic));
        const char *suffix = first_slash + 1;
        if (suffix[0] == '\0')
            return prefix + "/" + board_id_hex;
        return prefix + "/" + board_id_hex + "/" + suffix;
    }

    bool publishPayloadToTopicList(const String &payload, const String &topic_list)
    {
        if (topic_list.isEmpty())
            return publishPayload(payload, mqtt_default_topic);

        bool ok = true;
        int start = 0;
        while (start < static_cast<int>(topic_list.length()))
        {
            int end = topic_list.indexOf('\n', start);
            if (end < 0)
                end = topic_list.length();
            String topic = topic_list.substring(start, end);
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
    LoraMailboxMqtt(const char *server = settings::mqtt::server,
                    int port = settings::mqtt::port,
                    const char *topic = settings::mqtt::topic)
        : mqtt_server(server), mqtt_port(port), mqtt_default_topic(topic)
    {
        if (settings::mqtt::use_tls)
            mqtt_uri = String("mqtts://") + mqtt_server + ":" + mqtt_port;
        else
            mqtt_uri = String("mqtt://") + mqtt_server + ":" + mqtt_port;
    }

    void reconnect()
    {
        if (!settings::mqtt::enabled)
            return;

        if (!ensureWiFiConnected())
            return;

        if (mqtt_started || mqtt_connected)
            return;

        client.connect();
        mqtt_started = true;
    }

    void begin()
    {
        if (!settings::mqtt::enabled)
            return;

        client.setServer(mqtt_uri.c_str());
        if (settings::mqtt::use_tls)
            client.setCACert(MQTT_ROOT_CA);
        if (strlen(settings::mqtt::username) > 0)
            client.setCredentials(settings::mqtt::username, settings::mqtt::password);
        client.setBufferSize(1024);
        client.setAutoReconnect(true);
        client.onConnect([this](bool session_present)
                         {
                             mqtt_connected = true;
                             mqtt_started = true;
                             Serial.printf("MQTT connected, session_present=%d\n", session_present);

                             if (!pending_payload.isEmpty())
                             {
                                 String payload = pending_payload;
                                 String topics = pending_topics;
                                 pending_payload = "";
                                 pending_topics = "";
                                 publishPayloadToTopicList(payload, topics);
                             }
                         });
        client.onDisconnect([this](bool session_present)
                            {
                                mqtt_connected = false;
                                mqtt_started = false;
                                Serial.printf("MQTT disconnected, session_present=%d\n", session_present);
                            });
        client.onError([this](esp_mqtt_error_codes_t error)
                       {
                           mqtt_connected = false;
                           Serial.printf("MQTT error: type=%d connect_return=%d esp_tls_last=0x%x tls_stack=0x%x sock_errno=%d\n",
                                         error.error_type,
                                         error.connect_return_code,
                                         error.esp_tls_last_esp_err,
                                         error.esp_tls_stack_err,
                                         error.esp_transport_sock_errno);
                       });
        reconnect();
    }

    void sendMsg(JsonDocument json_doc)
    {
        if (!settings::mqtt::enabled)
            return;

        if (!ensureWiFiConnected())
            return;

        String mqtt_string;
        serializeJson(json_doc, mqtt_string);
        const char *topics[MAX_TOPICS_PER_MESSAGE];
        size_t topic_count = resolveTopics(json_doc, topics, MAX_TOPICS_PER_MESSAGE);
        const char *board_id_hex = json_doc["TX"]["TX_BOARD_ID"] | "";
        if (board_id_hex[0] == '\0')
            board_id_hex = json_doc["TX"]["BOARD_ID_HEX"] | "";

        String final_topics[MAX_TOPICS_PER_MESSAGE];
        size_t final_topic_count = 0;
        String topic_list;
        for (size_t i = 0; i < topic_count; ++i)
        {
            String routed_topic = topicWithBoardId(topics[i], board_id_hex);
            addTopicStringIfUnique(routed_topic, final_topics, final_topic_count, MAX_TOPICS_PER_MESSAGE);
        }

        for (size_t i = 0; i < final_topic_count; ++i)
        {
            if (i > 0)
                topic_list += '\n';
            topic_list += final_topics[i];
        }

        reconnect();
        if (!mqtt_connected)
        {
            pending_payload = mqtt_string;
            pending_topics = topic_list;
            Serial.printf("MQTT publish deferred: connected=%d payload=%u topic_count=%u uri=%s\n",
                          mqtt_connected,
                          static_cast<unsigned>(mqtt_string.length()),
                          static_cast<unsigned>(final_topic_count),
                          mqtt_uri.c_str());
            return;
        }

        publishPayloadToTopicList(mqtt_string, topic_list);
    }
};
