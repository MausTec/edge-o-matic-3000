#ifndef __config_h
#define __config_h

#include "errors.h"
#include "VibrationModeController.h"

#include <ArduinoJson.h>

// External Library Config
#define FASTLED_INTERNAL

// SD card files:
#define CONFIG_FILENAME "/config.json"
#define UPDATE_FILENAME "/update.bin"

// Uncomment to enable debug logging and functions.
//#define DEBUG

// Uncomment if compiling for NoGasm+
//#define NG_PLUS

// Butt Pin
#define MOT_PWM_PIN     15

// SD Connections
#define SD_CS_PIN       5

// Encoder Connection
#define ENCODER_B_PIN   32
#define ENCODER_A_PIN   33
#define ENCODER_RD_PIN  2
#define ENCODER_BL_PIN  27
#define ENCODER_GR_PIN  4

#ifdef NG_PLUS
  // LCD Definitions
  #define SCREEN_WIDTH 128
  #define SCREEN_HEIGHT 32

  // LED Ring (NoGasm+)
  #define LED_PIN 17
  #define LED_COUNT 13

  #define SDA_PIN 21
  #define SCL_PIN 22

  // Encoders are swapped
  #define ENCODER_GR_PIN 27
  #define ENCODER_BL_PIN 4
#else
  // LCD Definitions
  #define SCREEN_WIDTH    128
  #define SCREEN_HEIGHT   64
  #define OLED_DC         13
  #define OLED_RESET      14
  #define OLED_CS         12
#endif

#define WIFI_SSID_MAX_LEN 64
#define WIFI_KEY_MAX_LEN 64

bool atob(const char *a);

union ConfigValue {
  byte v_byte;
  int  v_int;
  char *v_char;
  bool v_bool;
};

struct ConfigStruct {
  // Networking
  char wifi_ssid[WIFI_SSID_MAX_LEN + 1];
  char wifi_key[WIFI_KEY_MAX_LEN + 1];
  bool wifi_on;

  char bt_display_name[64];
  bool bt_on;
  bool force_bt_coex;

  // UI And Stuff
  byte led_brightness;
  int screen_dim_seconds;
  int screen_timeout_seconds;

  // Server
  int websocket_port;
  bool classic_serial;
  bool use_ssl;
  char hostname[64];

  // Orgasms and Stuff
  byte motor_max_speed;
  byte motor_start_speed;
  int edge_delay;
  int max_additional_delay;
  int minimum_on_time;
  byte pressure_smoothing;
  int sensitivity_threshold;
  int motor_ramp_time_s;
  int update_frequency_hz;
  byte sensor_sensitivity;
  bool use_average_values;

  // Vibration Output Mode
  VibrationMode vibration_mode;
  
  // Post orgasm torure stuff
  int clench_pressure_sensitivity;
  int max_clench_duration; 
  int clench_threshold_2_orgasm;
  bool clench_detector_in_edging;

  int auto_edging_duration_minutes;
  int post_orgasm_duration_seconds;
  bool edge_menu_lock;
  bool post_orgasm_menu_lock;
};

extern ConfigStruct Config;


extern void loadConfigFromSd();
extern void loadDefaultConfig();
extern void loadConfigFromJsonObject(JsonDocument &doc);
extern void saveConfigToSd(long save_at_ms = -1);
extern bool setConfigValue(const char *key, const char *value, bool &require_reboot);
extern bool getConfigValue(const char *key, String &out);
extern void dumpConfigToJsonObject(JsonDocument &doc);
extern bool dumpConfigToJson(String &str);
#endif
