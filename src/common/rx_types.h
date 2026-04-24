/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <stdint.h>
#include <string.h>

enum class RxTrigger : uint8_t
{
    LoraPayloadReceived,
    HeartbeatRx,
    Boot,
    WifiReconnected,
};

static inline const char *rxTriggerToString(RxTrigger trigger)
{
    switch (trigger)
    {
    case RxTrigger::LoraPayloadReceived:
        return "LORA_PAYLOAD_RECEIVED";
    case RxTrigger::HeartbeatRx:
        return "HEARTBEAT_RX";
    case RxTrigger::Boot:
        return "RX_BOOT";
    case RxTrigger::WifiReconnected:
        return "RX_WIFI_RECONNECTED";
    default:
        return "UNKNOWN";
    }
}

static inline RxTrigger rxTriggerFromString(const char *trigger)
{
    if (trigger == nullptr)
        return RxTrigger::Boot;
    if (strcmp(trigger, "LORA_PAYLOAD_RECEIVED") == 0)
        return RxTrigger::LoraPayloadReceived;
    if (strcmp(trigger, "HEARTBEAT_RX") == 0)
        return RxTrigger::HeartbeatRx;
    if (strcmp(trigger, "RX_WIFI_RECONNECTED") == 0)
        return RxTrigger::WifiReconnected;
    return RxTrigger::Boot;
}
