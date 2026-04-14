/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "common/LoraMailBox_TX_types.h"

/**
 * @brief NTFY message priority.
 * @see https://docs.ntfy.sh/publish/#message-priority
 */
enum class NTFYPriority : uint8_t
{
    Min = 1,
    Low = 2,
    Default = 3,
    High = 4,
    Max = 5,
};

struct NTFYMessage
{
    TxTrigger tx_trigger;
    NTFYPriority priority;
    String title;
    String body;
};

static inline const char *ntfyPriorityToString(NTFYPriority priority)
{
    switch (priority)
    {
    case NTFYPriority::Min:
        return "min";
    case NTFYPriority::Low:
        return "low";
    case NTFYPriority::Default:
        return "default";
    case NTFYPriority::High:
        return "high";
    case NTFYPriority::Max:
        return "max";
    default:
        return "default";
    }
}
