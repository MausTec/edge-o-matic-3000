#ifndef __config_h
#define __config_h

#include "arduino.h"

#define CONFIG_FILENAME "/config.json"

// Uncomment to enable debug logging and functions.
#define DEBUG

// Uncomment if compiling for NoGasm+
//#define NG_PLUS

// Butt Pin
#define BUTT_PIN        34
#define MOT_PWM_PIN     15

// LCD Definitions
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_DC         13
#define OLED_RESET      14
#define OLED_CS         12

// SD Connections
#define SD_CS_PIN       5

#ifndef NG_PLUS
  // External Bus (NoGasm WiFi)
  #define BUS_EN_PIN      20
  #define RJ_LED_1_PIN    17
  #define RJ_LED_2_PIN    16
#else
  // LED Ring (NoGasm+)
  #define LED_PIN 17
  #define LED_COUNT 13
#endif

// Encoder Connection
#define ENCODER_B_PIN   32
#define ENCODER_A_PIN   33
#define ENCODER_SW_PIN  35
#define ENCODER_RD_PIN  2
#define ENCODER_GR_PIN  27
#define ENCODER_BL_PIN  4

// Buttons
#define KEY_1_PIN       39
#define KEY_2_PIN       25
#define KEY_3_PIN       26

struct ConfigStruct {
  // Networking
  char wifi_ssid[256];
  char wifi_key[256];
  bool wifi_on;

  char bt_display_name[256];
  bool bt_on;

  // UI And Stuff
  byte led_brightness;
  int screen_dim_seconds;

  // Server
  int websocket_port;
  bool classic_serial;

  // Orgasms and Stuff
  byte motor_max_speed;
  byte pressure_smoothing;
  int sensitivity_threshold;
  int motor_ramp_time_s;
  int update_frequency_hz;
  byte sensor_sensitivity;
} extern Config;

extern void loadConfigFromSd();
extern void saveConfigToSd(long save_at_ms = 0);
#endif