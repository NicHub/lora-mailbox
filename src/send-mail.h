/**
 * SEND MAIL
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

/*
  Rui Santos
  Complete project details at:
   - ESP32: https://RandomNerdTutorials.com/esp32-send-email-smtp-server-arduino-ide/
   - ESP8266: https://RandomNerdTutorials.com/esp8266-nodemcu-send-email-smtp-server-arduino/

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  Example adapted from: https://github.com/mobizt/ESP-Mail-Client
*/
#if (RXorTX == 0)
#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <ESP_Mail_Client.h>
#include "credentials.h"

uint64_t now_us = 0;
uint32_t MSG_CNT = 0;

/* Declare the global used SMTPSession object for SMTP transport */
SMTPSession smtp;

/*
 * Callback function to get the Email sending status
 */
void smtpCallback(SMTP_Status status)
{
    /* Print the current status */
    Serial.println(status.info());

    /* Print the sending result */
    if (status.success())
    {
        // ESP_MAIL_PRINTF used in the examples is for format printing via debug Serial port
        // that works for all supported Arduino platform SDKs e.g. AVR, SAMD, ESP32 and ESP8266.
        // In ESP8266 and ESP32, you can use Serial.printf directly.

        Serial.println("----------------");
        ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
        ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
        Serial.println("----------------\n");

        for (size_t i = 0; i < smtp.sendingResult.size(); i++)
        {
            /* Get the result item */
            SMTP_Result result = smtp.sendingResult.getItem(i);

            // In case, ESP32, ESP8266 and SAMD device, the timestamp get from result.timestamp should be valid if
            // your device time was synched with NTP server.
            // Other devices may show invalid timestamp as the device time was not set i.e. it will show Jan 1, 1970.
            // You can call smtp.setSystemTime(xxx) to set device time manually. Where xxx is timestamp (seconds since Jan 1, 1970)

            ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
            ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
            ESP_MAIL_PRINTF("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
            ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
            ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
        }
        Serial.println("----------------\n");

        // You need to clear sending result as the memory usage will grow up.
        smtp.sendingResult.clear();
    }
}

void sendMail()
{
    /*  Set the network reconnection option */
    MailClient.networkReconnect(true);

    /** Enable the debug via Serial port
     * 0 for no debugging
     * 1 for basic level debugging
     *
     * Debug port can be changed via ESP_MAIL_DEFAULT_DEBUG_PORT in ESP_Mail_FS.h
     */
    smtp.debug(1);

    /* Set the callback function to get the sending results */
    smtp.callback(smtpCallback);

    /* Declare the Session_Config for user defined session credentials */
    Session_Config config;

    /* Set the session config */
    config.server.host_name = SMTP_HOST;
    config.server.port = SMTP_PORT;
    config.login.email = SENDER_EMAIL;
    config.login.password = SENDER_PASSWORD;
    config.login.user_domain = "";

    /*
    Set the NTP config time
    For times east of the Prime Meridian use 0-12
    For times west of the Prime Meridian add 12 to the offset.
    Ex. American/Denver GMT would be -6. 6 + 12 = 18
    See https://en.wikipedia.org/wiki/Time_zone for a list of the GMT/UTC timezone offsets
    */
    config.time.ntp_server = F("ntp.metas.ch,0.pool.ntp.org,time.nist.gov");
    config.time.gmt_offset = 0;
    config.time.day_light_offset = 0;

    /* Declare the message class */
    SMTP_Message message;

    esp_reset_reason_t reset_reason = esp_reset_reason();

    String txtSender = String(SENDER_NAME);
    txtSender.concat(String(WiFi.macAddress()));

    String textMsg = String("CNT: ");
    textMsg.concat(++MSG_CNT);
    textMsg.concat(" - RESET REASON: ");
    textMsg.concat(reset_reason);
    textMsg.concat(" - SENDER: ");
    textMsg.concat(txtSender);
    textMsg.concat(" - TIMER us: ");
    textMsg.concat(now_us);

    String txtSubject = "pobox";

    /* Set the message headers */
    message.sender.name = txtSender;
    message.sender.email = SENDER_EMAIL;
    message.subject = txtSubject.c_str();

    // Add all recipients from array
    for (const auto& recipient : RECIPIENTS) {
        message.addRecipient(recipient.name, recipient.email);
    }

    /*Send HTML message*/
    /*String htmlMsg = "<div style=\"color:#2f4468;\"><h1>Hello World!</h1><p>- Sent from ESP board</p></div>";
    message.html.content = htmlMsg.c_str();
    message.html.content = htmlMsg.c_str();
    message.text.charSet = "us-ascii";
    message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;*/

    // Send raw text message
    message.text.content = textMsg.c_str();
    message.text.charSet = "us-ascii";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
    message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

    /* Connect to the server */
    if (!smtp.connect(&config))
    {
        ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
        return;
    }

    if (!smtp.isLoggedIn())
    {
        Serial.println("\nNot yet logged in.");
    }
    else
    {
        if (smtp.isAuthenticated())
            Serial.println("\nSuccessfully logged in.");
        else
            Serial.println("\nConnected with no Auth.");
    }

    /* Start sending Email and close the session */
    if (!MailClient.sendMail(&smtp, &message))
        ESP_MAIL_PRINTF("Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
}

void setupWifi()
{
    Serial.println();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
}

void printSplashScreen()
{
    Serial.println("\n\n##########################");
    Serial.print(F("# PROJECT PATH:     "));
    Serial.println(PROJECT_PATH);
    Serial.print(F("# COMPILATION DATE: "));
    Serial.println(COMPILATION_DATE);
    Serial.print(F("# COMPILATION TIME: "));
    Serial.println(COMPILATION_TIME);
    Serial.print(F("# F_CPU:            "));
    Serial.println(F_CPU);
    Serial.print(F("# LAST_COMMIT_ID:   "));
    Serial.println(LAST_COMMIT_ID);
    Serial.println("##########################\n\n");
}

// void setupSerial()
// {
//     Serial.begin(BAUD_RATE);
//     delay(2000);
//     Serial.print("\n\n\n");
//     Serial.flush();
//     printSplashScreen();
// }

void waitNextPeriod(uint64_t period_us)
{
    now_us = micros();
    uint64_t wait_us = period_us - now_us % period_us;
    Serial.printf("\n#### NOW BEFORE DELAY = %14lu\n", now_us);
    Serial.printf("#### WAIT             = %14lu\n", wait_us);
    Serial.printf("#### LAST DURATION    = %14lu\n", period_us - wait_us);
    Serial.flush();
    esp_sleep_enable_timer_wakeup(wait_us);
    esp_light_sleep_start();
    // setupWifi();
    Serial.printf("#### NOW AFTER DELAY  = %14lu\n", now_us);
}

// void setup()
// {
//     // while(true) yield();
//     setupSerial();
//     setupWifi();
// }

// void loop()
// {
//     sendMail();
// #define PERIOD_us (5ULL * 60000ULL * 1000ULL)
//     waitNextPeriod(PERIOD_us);
// }
#endif