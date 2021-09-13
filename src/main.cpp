// Dependancies
#include "defenitions.h"
#include <LITTLEFS.h>
#include <ArduinoJson.h>
#include <math.h>
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>
#include <ESP32Servo.h>

// Instances
Servo pwmThrottle;
TaskHandle_t server_Handle;
TaskHandle_t pwm_Handle;
QueueHandle_t server2PWM_QueueHandle;

//EStop
void toggleEStop(bool const &eStop) {
    xQueueSend(server2PWM_QueueHandle, &(eStop), 0);
}

// PWM Initialization
void pwmSetup() {
    // Initialize throttle
    pinMode(PWM_THROTTLE, OUTPUT);
    pwmThrottle.attach(PWM_THROTTLE);
    pwmThrottle.writeMicroseconds(IDLE_THROT);
    
    // Initialize steering
    pinMode(PWM_STEERING, OUTPUT);
    ledcSetup(STEERING_CHN, 300, 8);
    ledcAttachPin(PWM_STEERING, STEERING_CHN);
    ledcWrite(STEERING_CHN, MID_STEER);
    
    //enable panic so ESP32 restarts
    esp_task_wdt_init(WDT_TIMEOUT, true); 
    
    // Delay to calibrate ESC
    delay(7000);
    Serial.println("PWM Ready to go");
}

// PWM Loop
void pwmTasks(void* param) {
    // Init PWM
    pwmSetup();
    Serial.print("pwmTasks running on core ");
    Serial.println(xPortGetCoreID());
    
    bool killed;
    unsigned long begin = millis();
    unsigned long end = begin;
    bool i = false;
    while(true)
    {   Serial.print("pwm");
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
        
        // HeartBeat - Currently Checking Any Lapse In Communication
        while(!Serial.available() && (end-begin) < 200) 
        {end = millis();}
        if(end-begin >= 200) {
            Serial.println("Enterning backup routine");
            while(1) {
                pwmThrottle.writeMicroseconds(IDLE_THROT);
                ledcWrite(STEERING_CHN, MID_STEER);
                delay(100);
            }
        }
        
        // Get Message
        String payload;
        if(Serial.available()) {
            begin = millis();
            end = millis();
            payload = Serial.readStringUntil( '\n' );
        }
  
        // Deserialize Message
        const uint8_t size = JSON_OBJECT_SIZE(500);
        StaticJsonDocument<size> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {    
        } else {
            // Calculate Steering and Throttle
            float normalized_throttle = doc["throttle"];
            float normalized_steering = doc["steering"];
            int steering_pwm = !i? MID_STEER+int(normalized_steering*int((RIGHT_STEER-LEFT_STEER)/2)): MID_STEER;
            int throttle_pwm = !i? IDLE_THROT+int(normalized_throttle*500): IDLE_THROT;
            ledcWrite(STEERING_CHN, steering_pwm);
            pwmThrottle.writeMicroseconds(throttle_pwm);
            Serial.print("steering_pwm=");
            Serial.print(steering_pwm);
            Serial.print(" throttle_pwm=");
            Serial.println(throttle_pwm);
        }
        Serial.print("=>");
        delay(400);
    }
}

// Set Up
void setup()
{   
    host_wifi_config_t host_wifi_config;
    String host = "robotman";
    String ssid = "charman";
    String pass = "password";
    host_wifi_config.host = host;
    host_wifi_config.pass = pass;
    host_wifi_config.ssid = ssid;

    Serial.begin(115200);

    //Wifi and Host Configuration 
    if (isHostWifiConfigured()) { 
        setHostWifiConfig(&host_wifi_config);
    }
    else {
        initHostWifiConfig(true); //clear and init   
    }
    printHostWifiConfig();

    // Create Server to PWM Queue
    Serial.print("setup running on core ");
    Serial.println(xPortGetCoreID());
    server2PWM_QueueHandle = xQueueCreate(QUEUE_SIZE, sizeof(bool));
    if (!server2PWM_QueueHandle) {
        Serial.println("QUEUE NOT SET");
    }
    LITTLEFS.begin();
    initEventHandler();
    xTaskCreatePinnedToCore(webServer, "ServerTasks", 10000, NULL, 2, &server_Handle, 0);
    xTaskCreatePinnedToCore(pwmTasks, "pwmTasks", 10000, NULL, 2, &pwm_Handle, 1);
}

// Main Loop
void loop() {}
