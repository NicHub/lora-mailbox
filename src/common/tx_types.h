/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <stdint.h>

enum class TxTrigger : uint8_t
{
    Boot,
    WakeupPinHigh,
    HeartbeatTx,
};

static inline const char *txTriggerToString(TxTrigger trigger)
{
    switch (trigger)
    {
    case TxTrigger::Boot:
        return "BOOT";
    case TxTrigger::WakeupPinHigh:
        return "WAKEUP_PIN_HIGH";
    case TxTrigger::HeartbeatTx:
        return "HEARTBEAT_TX";
    default:
        return "UNKNOWN";
    }
}

