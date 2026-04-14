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
    static constexpr const char *MQTT_ROOT_CA = settings::mqtt::ROOT_CA_PEM;

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

    bool ensureWiFiConnected();
    void addTopicIfUnique(const char *topic, const char **topics, size_t &topic_count, size_t max_topics) const;
    void addTopicStringIfUnique(const String &topic, String *topics, size_t &topic_count, size_t max_topics) const;
    size_t resolveTopics(const JsonDocument &json_doc, const char **topics, size_t max_topics) const;
    bool publishPayload(const String &payload, const char *topic);
    String topicWithBoardId(const char *topic, const char *board_id_hex) const;
    bool publishPayloadToTopicList(const String &payload, const String &topic_list);

public:
    LoraMailboxMqtt(const char *server = settings::mqtt::SERVER,
                    int port = settings::mqtt::PORT,
                    const char *topic = settings::mqtt::TOPIC);

    void reconnect();
    void begin();
    void sendMsg(JsonDocument json_doc);
};
