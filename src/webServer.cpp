// Webserver Dependencies
#include "defenitions.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Webserver Instances
AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");

// WebSocket Event Handler
void webSocketEvent(AsyncWebSocket *server, 
                    AsyncWebSocketClient *client, 
                    AwsEventType type, 
                    void *arg, 
                    uint8_t *data, 
                    size_t len) {

    // New User Connection
    if(type == WS_EVT_CONNECT) {
        char jsonBuffer[JSON_SIZE];
        Serial.print("Web Server Connecting On Core[");
        Serial.print(xPortGetCoreID());
        Serial.println("]");
        Serial.print("Web Server Retrieving On Core[");
        getInitialValues(jsonBuffer);
        client->text(jsonBuffer);
    }
    // Incoming User Message
    else if(type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if(info->final && info->index==0 && info->len==len && info->opcode==WS_TEXT) {
            Serial.print("Web Server Recieving On Core[");
            Serial.print(xPortGetCoreID());
            Serial.println("]");
            Serial.print("WebServer Parsing On Core[");
            parseData(data);
        }
    }
}

// Send Sync Message To All Devices    
void syncDevices(char *message) {
    ws.textAll(message);           
}

// Web Server Loop
void webServer(void* param) {    
    // Initialize Web Socket Handlers
    ws.onEvent(webSocketEvent);
    server.addHandler(&ws);
    
    // Initialize Web Server Handlers
    server.serveStatic("/", LITTLEFS, "/");
    server.on("/", [](AsyncWebServerRequest *request){
        request->send(LITTLEFS, "/index.html", "text/html");
    });
    
    // Launch Soft Access Point and Server
    WiFi.softAP(WIFI_SSID, WIFI_PASS);
    server.begin();
    
    // Main Web Server Loop
    Serial.print("Web Server Running On Core[");
    Serial.print(xPortGetCoreID());
    Serial.println("]");
    while (true)
    {   // Close Lingering WebSockets
        if(millis()%WEBSOCKET_TIMEOUT == 0) { 
            ws.cleanupClients();
        }
        
        // ESP32 Crashes Without Thread Delay
        delay(WEBSERVER_DELAY);
    }
}