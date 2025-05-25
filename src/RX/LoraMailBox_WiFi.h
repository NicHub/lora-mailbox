/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "credentials.h"

class LoraMailBox_WiFi
{
private:
    AsyncWebServer server{80};
    AsyncWebSocket ws{"/ws"};
    String latestMessage = "";
    const char *htmlTemplate = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>LoRa MailBox</title>
    <style>
        :root {
            --bg: #fff;
            --text: #212121;
            --primary: #1976d2;
            --secondary: #f5f5f5;
            --border: #e0e0e0;
        }
        @media (prefers-color-scheme: dark) {
            :root {
                --bg: #212121;
                --text: #f5f5f5;
                --primary: #64b5f6;
                --secondary: #333;
                --border: #444;
            }
        }
        body {
            font-family: system-ui, sans-serif;
            line-height: 1.6;
            max-width: 800px;
            margin: 0 auto;
            padding: 1rem;
            color: var(--text);
            background: var(--bg);
        }
        h1 {
            color: var(--primary);
        }
        pre {
            background: var(--secondary);
            border: 1px solid var(--border);
            border-radius: 10px;
            padding: 1rem;
            overflow-x: auto;
            min-height: 200px;
        }
    </style>
</head>
<body>
    <h1>LoRa MailBox</h1>
    <pre id="messages">Connecting to WebSocket...</pre>

    <script>
        const gateway = `ws://${window.location.hostname}/ws`;
        let websocket;

        function initWebSocket() {
            websocket = new WebSocket(gateway);
            websocket.onopen = () => console.log('Connection opened');
            websocket.onclose = () => {
                console.log('Connection closed');
                setTimeout(initWebSocket, 2000);
            };
            websocket.onmessage = (event) => {
                document.getElementById('messages').innerHTML = event.data;
            };
        }

        window.addEventListener('load', initWebSocket);
    </script>
</body>
</html>
)rawliteral";

public:
    LoraMailBox_WiFi() {}

    bool begin()
    {
        // Connect to WiFi.
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        // Wait for connection.
        uint8_t attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20)
        {
            delay(500);
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("\nWiFi connection failed!");
            return false;
        }

        Serial.println("\nConnected to WiFi");
        Serial.print("IP address: \nhttp://");
        Serial.println(WiFi.localIP());

        // Set up WebSocket handler.
        ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client,
                          AwsEventType type, void *arg, uint8_t *data, size_t len)
                   {
            if (type == WS_EVT_CONNECT) {
                Serial.printf("WebSocket client #%u connected\n", client->id());
                if (latestMessage.length() > 0) {
                    client->text(latestMessage);
                }
            } });

        server.addHandler(&ws);

        // Route for root / web page.
        server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
                  { request->send(200, "text/html", htmlTemplate); });

        // Start server.
        server.begin();
        return true;
    }

    void sendWsMsg(const String &message)
    {
        latestMessage = message;
        ws.textAll(latestMessage);
        // ws.cleanupClients();
    }

    IPAddress getLocalIP()
    {
        return WiFi.localIP();
    }
};
