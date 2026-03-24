/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <stdint.h>

enum class NTFYMessageKind : uint8_t
{
    None,
    MessageReceived,
    Heartbeat,
};

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
    NTFYMessageKind kind;
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
