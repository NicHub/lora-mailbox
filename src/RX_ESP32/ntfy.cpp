/**
 * LoRa MailBox — NTFY
 *
 * https://ntfy.sh/
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include "ntfy.h"

NTFYPriority LoraMailboxNtfy::getNTFYPriority(TxTrigger tx_trigger) const
{
    switch (tx_trigger)
    {
    case TxTrigger::WakeupPinHigh:
        return settings::ntfy::MESSAGE_RECEIVED_PRIORITY;
    case TxTrigger::HeartbeatTx:
        return settings::ntfy::HEARTBEAT_PRIORITY;
    case TxTrigger::Boot:
        return settings::ntfy::BOOT_PRIORITY;
    default:
        return NTFYPriority::Default;
    }
}

const char *LoraMailboxNtfy::getNTFYIcon(TxTrigger tx_trigger) const
{
    switch (tx_trigger)
    {
    case TxTrigger::WakeupPinHigh:
        return settings::ntfy::MESSAGE_RECEIVED_ICON;
    case TxTrigger::HeartbeatTx:
        return settings::ntfy::HEARTBEAT_ICON;
    case TxTrigger::Boot:
        return settings::ntfy::BOOT_ICON;
    default:
        return "";
    }
}

const char *LoraMailboxNtfy::getNTFYTitleSuffix(TxTrigger tx_trigger) const
{
    switch (tx_trigger)
    {
    case TxTrigger::WakeupPinHigh:
        return settings::ntfy::MESSAGE_RECEIVED_TITLE_SUFFIX;
    case TxTrigger::HeartbeatTx:
        return settings::ntfy::HEARTBEAT_TITLE_SUFFIX;
    case TxTrigger::Boot:
        return settings::ntfy::BOOT_TITLE_SUFFIX;
    default:
        return "";
    }
}

TxTrigger LoraMailboxNtfy::getNTFYTrigger(const JsonDocument &json_doc) const
{
    if (!settings::ntfy::ENABLED)
        return TxTrigger::Boot;

    const char *tx_trigger = json_doc["TX"]["TX_TRIGGER"] | "";
    if (strcmp(tx_trigger, "WAKEUP_PIN_HIGH") == 0)
        return TxTrigger::WakeupPinHigh;
    if (settings::ntfy::NOTIFY_HEARTBEAT_TX && strcmp(tx_trigger, "HEARTBEAT_TX") == 0)
        return TxTrigger::HeartbeatTx;
    return TxTrigger::Boot;
}

String LoraMailboxNtfy::buildNTFYBody(const JsonDocument &json_doc) const
{
    if (!settings::ntfy::ENABLED)
    {
        (void)json_doc;
        return "";
    }

    uint16_t counter_value = json_doc["RX"]["RX_COUNTER"] | 0;
    const char *counter_status = json_doc["RX_TX"]["RX_TX_COUNTER_STATUS"] | "";
    uint16_t tx_vbat_raw = json_doc["TX"]["TX_VBAT_RAW"] | 0;
    int tx_vbat_mv = json_doc["TX"]["TX_VBAT_MV"] | 0;
    int tx_vbat_percent = json_doc["TX"]["TX_VBAT_PERCENT"] | 0;
    const char *tx_vbat_glyph = json_doc["TX"]["TX_VBAT_GLYPH"] | "";
    const char *tx_vbat_status = json_doc["TX"]["TX_VBAT_STATUS"] | "";
    float rx_tx_rssi_dbm = json_doc["RX_TX"]["RX_TX_RSSI_DBM"] | 0.0f;
    float rx_tx_snr_db = json_doc["RX_TX"]["RX_TX_SNR_DB"] | 0.0f;
    const char *md_code_tag = settings::ntfy::MD_CODE_TAG;

    String text;
    text += md_code_tag;
    text += "bat ";
    text += tx_vbat_glyph;
    text += " ";
    text += String(tx_vbat_percent);
    text += "% (";
    text += String(tx_vbat_mv);
    text += "mV, ";
    text += tx_vbat_status;
    text += ", ";
    text += String(tx_vbat_raw);
    text += ")";
    text += md_code_tag;
    text += "\n";
    text += md_code_tag;
    text += "ctr ";
    text += String(counter_value);
    text += " (";
    text += counter_status;
    text += ")";
    text += md_code_tag;
    text += "\n";
    text += md_code_tag;
    text += "RSSI ";
    text += String(rx_tx_rssi_dbm, 0);
    text += "dBm ";
    text += " | ";
    text += "SNR ";
    text += String(rx_tx_snr_db, 1);
    text += "dB";
    text += md_code_tag;

    return text;
}

NTFYMessage LoraMailboxNtfy::buildNTFYMessage(const JsonDocument &json_doc) const
{
    if (!settings::ntfy::ENABLED)
    {
        (void)json_doc;
        return NTFYMessage{TxTrigger::Boot, NTFYPriority::Default, "", ""};
    }

    TxTrigger tx_trigger = getNTFYTrigger(json_doc);

    struct tm timeinfo;
    char time_str[9] = "";
    if (getLocalTime(&timeinfo))
        strftime(time_str, sizeof(time_str), "%H:%M:%S", &timeinfo);

    const char *ntfy_icon = getNTFYIcon(tx_trigger);
    const char *ntfy_title_suffix = getNTFYTitleSuffix(tx_trigger);
    String title;
    if (ntfy_icon[0] != '\0')
        title = String(ntfy_icon) + " ";
    if (time_str[0] != '\0')
        title += String(time_str);
    title += " @" + String(settings::misc::RECIPIENT_NAME);
    title += ntfy_title_suffix;

    return NTFYMessage{
        tx_trigger,
        getNTFYPriority(tx_trigger),
        title,
        buildNTFYBody(json_doc),
    };
}

bool LoraMailboxNtfy::sendMsg(const JsonDocument &json_doc, const String &topic)
{
    if (!settings::ntfy::ENABLED)
    {
        (void)json_doc;
        (void)topic;
        return false;
    }

    if (WiFi.status() != WL_CONNECTED)
        return false;

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    String url = String(settings::ntfy::SERVER) + topic;

    bool ok = false;
    if (!https.begin(client, url))
        return false;

    NTFYMessage ntfy_message = buildNTFYMessage(json_doc);
    https.addHeader("Title", ntfy_message.title);
    https.addHeader("Priority", ntfyPriorityToString(ntfy_message.priority));

    if (String(settings::ntfy::USERNAME).length() > 0 ||
        String(settings::ntfy::PASSWORD).length() > 0)
        https.setAuthorization(settings::ntfy::USERNAME, settings::ntfy::PASSWORD);

    https.addHeader("Markdown", "yes");
    https.addHeader("Content-Type", "text/markdown; charset=utf-8");
    int http_code = https.POST(ntfy_message.body);
    ok = (http_code >= 200 && http_code < 300);
    https.end();

    return ok;
}
