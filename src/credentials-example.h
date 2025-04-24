/*
    Rename this file as credentials.h and update the details below with your information.
*/

#pragma once

// Wifi credentials
#define WIFI_SSID "..."
#define WIFI_PASSWORD "..."

// Mail sender credentials
#define SMTP_HOST F("smtp.gmail.com")
#define SMTP_PORT 465
#define SENDER_NAME F("ESP-")
#define SENDER_EMAIL F("...@gmail.com")
#define SENDER_PASSWORD F("...")

// Recipients array
struct Recipient
{
    const char *name;
    const char *email;
};

const Recipient RECIPIENTS[] = {
    {"name 1",
     "mail1@domain.com"},
    {"name 2",
     "mail@domain.com"},
    {"name 3",
     "mail3@domain.com"},
};

#define RECIPIENT_COUNT (sizeof(RECIPIENTS) / sizeof(RECIPIENTS[0]))

// MQTT
const char *mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char *mqtt_topic = "yourtopic";
