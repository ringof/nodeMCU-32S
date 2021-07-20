// Dependancies
#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <math.h>
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>
#include <ESP32Servo.h>

// Macros
#define SERVO_PIN 0
#define ESC_PIN 4
#define EMO_PIN 15
#define HTTP_PORT 80
#define TIMEOUT 500
#define WDT_TIMEOUT 3
#define STEERING_CHN 3
#define IDLE_THROT 1500
#define BRAKE_THROT 1000
#define MID_THROT 1500
#define MAX_THROT 2000
#define LEFT_STEER 84
#define CENTER_STEER 116
#define RIGHT_STEER 148
#define QUEUE_SIZE 10
#define JSON_SIZE 11
#define BUFFER_SIZE 220

// WiFi Credentials
const char *WIFI_SSID = "ESP32Test";
const char *WIFI_PASS = "jetsonucsd";

// Button Component
struct Button {
    uint8_t pin;
    bool    on;
    void update() {digitalWrite(pin, on ? HIGH : LOW);}
};

// Instances
Button eStop_button = {EMO_PIN, false };
Button ai_button = {NULL, false };
AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");
Servo pwmThrottle;
TaskHandle_t server_Handle;
TaskHandle_t pwm_Handle;
QueueHandle_t server2PWM_QueueHandle;
QueueHandle_t server2Status_QueueHandle;

// FiLL In Buttons For New Users Using Current Values
String processor(const String &var) {
    String buttons = "";
    if(var == "E-STOP"){
        String eStop_state = eStop_button.on?"on":"off";
        buttons += "<button id='eStop' class="+eStop_state+"></button>";
    }
    return buttons;
}

// Locate HTML Page
void onRootRequest(AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html", false, processor);
}

// WebSocket Events
void onEvent(AsyncWebSocket       *server,
             AsyncWebSocketClient *client,
             AwsEventType          type,  
             void                 *arg,   
             uint8_t              *data,  
             size_t                len) { 

    // Incomings Message
    if(type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            // Deseralize Message
            const size_t size = JSON_OBJECT_SIZE(JSON_SIZE);
            StaticJsonDocument<size> json;
            DeserializationError err = deserializeJson(json, data);
            if(err){ return;}
            
            // Launch AI 
            if(strcmp(json["event"], "save") == 0) {
                //save data
            } 
            // Launch AI 
            else if(strcmp(json["event"], "ai") == 0) {
                ai_button.on = !ai_button.on;
                json["state"] = ai_button.on? "on": "off";
            } 
            // Update EMO Button
            else if(strcmp(json["event"], "eStop") == 0) {
                eStop_button.on = !eStop_button.on;
                json["state"] = eStop_button.on? "on": "off";
            }
            
            // Sync Devices
            char buffer[BUFFER_SIZE];
            ws.textAll(buffer, serializeJson(json, buffer));
        }
    }
}

// Server Initialization
void serverSetup() {
    // Define Pin Behavior
    pinMode(eStop_button.pin, OUTPUT);
    
    // Activate Serial Monitor 
    delay(500);
  
    // Start SPIFFS
    SPIFFS.begin();
    
    // Create Soft Access Point
    WiFi.softAP(WIFI_SSID, WIFI_PASS);
    
    // Start Web Socket
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    
    // Launch Web Server
    server.on("/", onRootRequest);
    server.serveStatic("/", SPIFFS, "/");
    server.begin();
    Serial.println("\nServer Ready to go");
}

// Server Loop
void serverTasks(void* param) {
    Serial.print("serverTasks running on core ");
    Serial.println(xPortGetCoreID());
    
    while (true)
    {   // Update E-Stop Button
        eStop_button.update();
    
        // Close Lingering WebSockets
        if(millis()%1000 == 0){ ws.cleanupClients();}
        delay(400);
    }
}

// PWM Initialization
void pwmSetup() {
    // Initialize throttle
    pinMode(ESC_PIN, OUTPUT);
    pwmThrottle.attach(ESC_PIN);
    pwmThrottle.writeMicroseconds(IDLE_THROT);
    
    // Initialize steering
    pinMode(SERVO_PIN, OUTPUT);
    ledcSetup(STEERING_CHN, 300, 8);
    ledcAttachPin(SERVO_PIN, STEERING_CHN);
    ledcWrite(STEERING_CHN, CENTER_STEER);
    
    //enable panic so ESP32 restarts
    esp_task_wdt_init(WDT_TIMEOUT, true); 
    
    // Delay to calibrate ESC
    delay(7000);
    Serial.println("PWM Ready to go");
}

// PWM Loop
void pwmTasks(void* param) {
    bool killed;
    Serial.print("pwmTasks running on core ");
    Serial.println(xPortGetCoreID());
    unsigned long begin = millis();
    unsigned long end = millis();
    esp_task_wdt_add(NULL);
    bool i = false;
    while(true)
    {    
        Serial.print("red button: "+Serial.available());
        //Serial.println(red_Button.on);
        delay(100);
        // Serial Check
    
        if (!server2PWM_QueueHandle) {
            Serial.println("QUEUE NOT SET");
        }
        if (xQueueReceive(server2PWM_QueueHandle, &i, 0) == pdTRUE) {
            Serial.print("BUTTON == ");
            Serial.println(i);
            if (i) {
                killed = true;
            } else {
                killed = false;
            }
            Serial.print("killed == ");
            Serial.println(killed);
        }
        
        // Heartbeat Check
        while(!Serial.available()) {
            end = millis();

            // Enter Backup Routine
            if(end-begin >= 200) {
                Serial.println("Enterning backup routine");
                while(1) {
                    pwmThrottle.writeMicroseconds(IDLE_THROT);
                    ledcWrite(STEERING_CHN, CENTER_STEER);
                    delay(100);
                }
            }
        }
        
        // Get Message
        String payload;
        if(Serial.available()) {
            begin = millis();
            end = millis();
            esp_task_wdt_reset();
            payload = Serial.readStringUntil( '\n' );
        }
  
        // Deserialize Message
        const uint8_t size = JSON_OBJECT_SIZE(1000);
        StaticJsonDocument<size> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {    
        } else {
            // Calculate Steering and Throttle
            float normalized_throttle = doc["throttle"];
            float normalized_steering = doc["steering"];
            int steering_pwm = !i? CENTER_STEER+int(normalized_steering*int((RIGHT_STEER-LEFT_STEER)/2)): CENTER_STEER;
            int throttle_pwm = !i? IDLE_THROT+int(normalized_throttle*500): IDLE_THROT;
            ledcWrite(STEERING_CHN, steering_pwm);
            pwmThrottle.writeMicroseconds(throttle_pwm);
            Serial.print("steering_pwm=");
            Serial.print(steering_pwm);
            Serial.print(" throttle_pwm=");
            Serial.println(throttle_pwm);
        };
    }
}

// Set Up
void setup()
{   // Start Server
    
    // Start PWM
    
    // Create Server to PWM Queue
    Serial.begin(115200);
    Serial.print("setup running on core ");
    Serial.println(xPortGetCoreID());
    server2PWM_QueueHandle = xQueueCreate(QUEUE_SIZE, sizeof(bool));
    server2Status_QueueHandle = xQueueCreate(QUEUE_SIZE, sizeof(int));
    if (!server2PWM_QueueHandle) {
        Serial.println("QUEUE NOT SET");
    }

    serverSetup();
    xTaskCreatePinnedToCore(serverTasks, "ServerTasks", 10000, NULL, 2, &server_Handle, 0);
    delay(500); 
    //pwmSetup();
    //xTaskCreatePinnedToCore(pwmTasks, "pwmTasks", 10000, NULL, 1, &pwm_Handle, 1);
    //delay(500);
}

// Main Loop
void loop() {}