#ifndef ESTOP_DEFINES_H
#define ESTOP_DEFINES_H

#include <Arduino.h>
#include <LITTLEFS.h>

struct host_wifi_config_t {
    String host;
    String ssid;
    String pass;
};

// -------------------------------------------------------------------
// I/O Pins (Hardcoded, PCB V1.0)
// -------------------------------------------------------------------

// Generic PWM Outputs
#define PWM_CH1 12

// PWM Pin Aliases
#define PWM_THROTTLE 2
#define PWM_STEERING 16

// Generic RC Inputs
#define RC_CH1 36
#define RC_CH2 39
#define RC_CH3 34
#define RC_CH4 35

// RGB LED
#define RGB_R 14
#define RGB_G 27
#define RGB_B 26

// Red/Blu LED
#define RB_R  4
#define RB_B  0

// BLDC Hall Effect Sensor Pins
#define BLDC1 32
#define BLDC2 33
#define BLDC3 25

// Generic I2C Bus x2
#define I2C1_SDA  21
#define I2C1_SCL  22
#define I2C2_SDA  15
#define I2C2_SCL  5

// Generic SPI Bus
#define SPI_MISO  19
#define SPI_MOSI  23
#define SPI_CLK   18
#define SPI_CS    17

// Output Enable (input pin)
// For emulating Adafruit PCA9685 PWM Driver, use with I2C1 bus
#define PWM_OE    13

// ---------------------------------------------------------
// Macros (TODO Organize)
// ---------------------------------------------------------
#define TIMEOUT 500
#define WDT_TIMEOUT 3
#define QUEUE_SIZE 10
#define BUFFER_SIZE 310
#define STEERING_CHN 3
#define EMO_PIN 15
// ---------------------------------------------------------
// Calibration Parameters (TODO Store in Flash Memory)
// ---------------------------------------------------------
#define IDLE_THROT  1500
#define BRAKE_THROT 1000 // Full Reverse?
#define MID_THROT   1750
#define MAX_THROT   2000

// Currently calculated from mapping 0-255 values
#define LEFT_STEER 84
#define MID_STEER 116
#define RIGHT_STEER 148


// Original 0-255 values. Remove eventually
//#define LEFT_STEER 84
//#define CENTER_STEER 116
//#define RIGHT_STEER 148

// ---------------------------------------------------------
// RTOS Structure Macros
// ---------------------------------------------------------

#define STACK_SIZE_SERVER   (1024U*10)   // 10 KWords
#define STACK_SIZE_PWM      (1024U*4)    //  4 KWords
#define STACK_SIZE_SERIAL   (1024U*4)    //  4 KWords
#define STACK_SIZE_JSON     (1024U*10)   // 10 KWords

#define QUEUE_LEN_PWM       (10U)
#define QUEUE_LEN_SERIAL    (10U)
#define QUEUE_LEN_JSON      (10U)

#define SERIAL_BUFSIZE      (256U)

// ---------------------------------------------------------
// Data Structs for IPC
// ---------------------------------------------------------

// Passed into qh_pwmCommand from Serial or WebServer;
// PWM Task receives PWM command and dispatches it
typedef struct {
  uint16_t  channel;      // PWM Channel [1-3]
  uint16_t  pulseWidth;   // Positive Pulse Width (millisec)
} pwm_cmd_t;

// Serial Buffer Pool Entry, used to allocate strings from
// static memory to reduce malloc() fragmentation risk
typedef struct {
  SemaphoreHandle_t   sem;
  char                buf[SERIAL_BUFSIZE];
} str_pool_ent_t;

extern str_pool_ent_t serBufPool[QUEUE_LEN_SERIAL];

// ---------------------------------------------------------
// RTOS Task Functions and Handles
// ---------------------------------------------------------

extern TaskHandle_t th_server;
extern TaskHandle_t th_pwm;
extern TaskHandle_t th_serial;
extern TaskHandle_t th_json;

extern QueueHandle_t qh_pwmCommand;
extern QueueHandle_t qh_jsonToParse;


// ---------------------------------------------------------
// ACCESS POINT
// ---------------------------------------------------------

  // Defenitions
  #define CONFIG_NS "config"
  #define DFLT_WIFI_SSID "robocar"
  #define DFLT_WIFI_PASS "jetsonucsd"
  #define DFLT_HOSTNAME "donkey"

// ---------------------------------------------------------
// WEB SERVER
// ---------------------------------------------------------

  // Defenitions 
  #define HTTP_PORT 80
  #define WEBSOCKET_TIMEOUT 1000
  #define WEBSERVER_DELAY 400

  // Functions 
  void webServer(void *param);
  void syncDevices(char *message);

// ---------------------------------------------------------
// EVENT HANDLER
// ---------------------------------------------------------

  // Defenitions 
  #define JSON_SIZE 510

  // Functions
  void initEventHandler();
  void getInitialValues(char *jsonBuffer);
  void parseData(uint8_t *data);

// ---------------------------------------------------------
// PWM GENERATOR
// ---------------------------------------------------------

  // Functions
  void initPWMGenerator();
  void pwmGenerator(void *param);
  void toggleEStop(bool const &eStop);
  void testCalibration( int const &ltSteer,
                        int const &ctrSteer,
                        int const &rtSteer,
                        int const &minThrot,
                        int const &midThrot,
                        int const &maxThrot,
                        int const &frequency);

// ---------------------------------------------------------
// JETSON SERVER
// ---------------------------------------------------------

  // Functions
  void task_SerialParser(void *param);
  void toggleAI(bool const &ai);
  void eraseRecords(int const &nRecords);
  void scaleThrottle(int const &tScalar);
  void updatePID( int const &kp,
                  int const &ki,
                  int const &kd);

// ---------------------------------------------------------
// Host / Wifi Configuration
// ---------------------------------------------------------

  // Functions
  bool isHostWifiConfigured();
  bool isDefaultHostWifiConfig();
  void initHostWifiConfig(bool);
  void printHostWifiConfig();
  bool getHostWifiConfig(host_wifi_config_t*);
  bool setHostWifiConfig(host_wifi_config_t*);

#endif