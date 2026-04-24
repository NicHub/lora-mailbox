/**
 * LoRa MailBox — MQTT
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include "mqtt.h"
#include "common/rx_types.h"

bool LoraMailboxMqtt::ensureWiFiConnected()
{
    if (WiFi.status() == WL_CONNECTED)
        return true;

    Serial.printf("WiFi not connected, status=%d\n", WiFi.status());
    return false;
}

void LoraMailboxMqtt::addTopicIfUnique(
    const char *topic, const char **topics, size_t &topic_count, size_t max_topics) const
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

void LoraMailboxMqtt::addTopicStringIfUnique(
    const String &topic, String *topics, size_t &topic_count, size_t max_topics) const
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

size_t LoraMailboxMqtt::resolveTopics(
    const JsonDocument &json_doc, const char **topics, size_t max_topics) const
{
    size_t topic_count = 0;
    RxTrigger rx_trigger = rxTriggerFromString(json_doc["RX"]["RX_TRIGGER"] | "");
    const char *tx_trigger = json_doc["TX"]["TX_TRIGGER"] | "";
    if (rx_trigger == RxTrigger::HeartbeatRx)
        addTopicIfUnique(settings::mqtt::TOPIC_HEARTBEAT_RX, topics, topic_count, max_topics);

    if (strcmp(tx_trigger, "WAKEUP_PIN_HIGH") == 0)
        addTopicIfUnique(settings::mqtt::TOPIC_GOT_MAIL, topics, topic_count, max_topics);

    if (strcmp(tx_trigger, "HEARTBEAT_TX") == 0)
        addTopicIfUnique(settings::mqtt::TOPIC_HEARTBEAT_TX, topics, topic_count, max_topics);

    if (strcmp(tx_trigger, "BOOT") == 0)
        addTopicIfUnique(settings::mqtt::TOPIC_WAKE_BOOT, topics, topic_count, max_topics);

    if (topic_count == 0)
        addTopicIfUnique(mqtt_default_topic, topics, topic_count, max_topics);
    return topic_count;
}

bool LoraMailboxMqtt::publishPayload(const String &payload, const char *topic)
{
    const int publish_result = client.publish(topic, 0, false, payload.c_str());
    if (publish_result < 0)
    {
        Serial.printf(
            "MQTT publish failed: connected=%d payload=%u topic=%s uri=%s\n",
            mqtt_connected,
            static_cast<unsigned>(payload.length()),
            topic,
            mqtt_uri.c_str());
        return false;
    }
    return true;
}

String LoraMailboxMqtt::topicWithBoardId(const char *topic, const char *board_id_hex) const
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

bool LoraMailboxMqtt::publishPayloadToTopicList(const String &payload, const String &topic_list)
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

LoraMailboxMqtt::LoraMailboxMqtt(const char *server, int port, const char *topic)
    : mqtt_server(server), mqtt_port(port), mqtt_default_topic(topic)
{
    if (settings::mqtt::USE_TLS)
        mqtt_uri = String("mqtts://") + mqtt_server + ":" + mqtt_port;
    else
        mqtt_uri = String("mqtt://") + mqtt_server + ":" + mqtt_port;
}

void LoraMailboxMqtt::reconnect()
{
    if (!settings::mqtt::ENABLED)
        return;

    if (!ensureWiFiConnected())
        return;

    if (mqtt_started || mqtt_connected)
        return;

    client.connect();
    mqtt_started = true;
}

void LoraMailboxMqtt::begin()
{
    if (!settings::mqtt::ENABLED)
        return;

    client.setServer(mqtt_uri.c_str());
    if (settings::mqtt::USE_TLS)
        client.setCACert(MQTT_ROOT_CA);
    if (strlen(settings::mqtt::USERNAME) > 0)
        client.setCredentials(settings::mqtt::USERNAME, settings::mqtt::PASSWORD);
    client.setBufferSize(1024);
    client.setAutoReconnect(true);
    client.onConnect(
        [this](bool session_present)
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
    client.onDisconnect(
        [this](bool session_present)
        {
            mqtt_connected = false;
            mqtt_started = false;
            Serial.printf("MQTT disconnected, session_present=%d\n", session_present);
        });
    client.onError(
        [this](esp_mqtt_error_codes_t error)
        {
            mqtt_connected = false;
            Serial.printf(
                "MQTT error: type=%d connect_return=%d esp_tls_last=0x%x tls_stack=0x%x "
                "sock_errno=%d\n",
                error.error_type,
                error.connect_return_code,
                error.esp_tls_last_esp_err,
                error.esp_tls_stack_err,
                error.esp_transport_sock_errno);
        });
    reconnect();
}

void LoraMailboxMqtt::sendMsg(JsonDocument json_doc)
{
    if (!settings::mqtt::ENABLED)
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
        addTopicStringIfUnique(
            routed_topic, final_topics, final_topic_count, MAX_TOPICS_PER_MESSAGE);
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
        Serial.printf(
            "MQTT publish deferred: connected=%d payload=%u topic_count=%u uri=%s\n",
            mqtt_connected,
            static_cast<unsigned>(mqtt_string.length()),
            static_cast<unsigned>(final_topic_count),
            mqtt_uri.c_str());
        return;
    }

    publishPayloadToTopicList(mqtt_string, topic_list);
}
