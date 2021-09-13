// Webserver Dependencies
#include "defenitions.h"
#include <ESPmDNS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>


// Host/Wifi Config
host_wifi_config_t host_wifi_config;

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
        getInitialValues(jsonBuffer);
        client->text(jsonBuffer);
    }
    // Incoming User Message
    else if(type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if(info->final && info->index==0 && info->len==len && info->opcode==WS_TEXT) {
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

    if (!getHostWifiConfig(&host_wifi_config)) {
        Serial.println("No Host/Wifi Configuration Found in EEPROM");
        return;
    }

    // Initialize Web Socket Handlers
    ws.onEvent(webSocketEvent);
    server.addHandler(&ws);
    
    // Initialize Web Server Handlers
    server.serveStatic("/", LITTLEFS, "/");
    server.on("/", [](AsyncWebServerRequest *request){
        request->send(LITTLEFS, "/index.html", "text/html");
    });
    
    WiFi.softAP(host_wifi_config.ssid.c_str(), host_wifi_config.pass.c_str());
    server.begin();
    
    // mDNS service

    if(!MDNS.begin(host_wifi_config.host.c_str())) {
        Serial.println("Error starting mDNS");
        return;
    }
    else {
        Serial.print("mDNS started: ");
        Serial.print(host_wifi_config.host.c_str());
        Serial.println(".local");
    }

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