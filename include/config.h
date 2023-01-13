#ifndef __config_h
#define __config_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// SD card files:
#define CONFIG_PATH_MAX 255

// This is just a default, others can be loaded after boot.
#define CONFIG_FILENAME "/config.json"

// String Lengths
#define WIFI_SSID_MAX_LEN 64
#define WIFI_KEY_MAX_LEN 64

// Vibration Modes
// See VibrationModeController.h for more.

enum vibration_mode {
  RampStop = 1,
  Depletion = 2,
  Enhancement = 3,
  Pattern = 4,
  GlobalSync = 0
};

typedef enum vibration_mode vibration_mode_t;

/**
 * Main Configuration Struct!
 * 
 * Place all presistent runtime config variables in here, and be sure to add the appropriate def
 * in config.c!
 */

struct config {
  // Private Things, do not erase!
  char _filename[CONFIG_PATH_MAX + 1];

  // Networking
  char wifi_ssid[WIFI_SSID_MAX_LEN + 1];
  char wifi_key[WIFI_KEY_MAX_LEN + 1];
  bool wifi_on;

  char bt_display_name[64];
  bool bt_on;
  bool force_bt_coex;

  // UI And Stuff
  uint8_t led_brightness;
  int screen_dim_seconds;
  int screen_timeout_seconds;
  uint8_t enable_screensaver;

  // Console
  bool store_command_history;
  bool console_basic_mode;

  // Server
  int websocket_port;
  bool classic_serial;
  bool use_ssl;
  char hostname[64];

  // Orgasms and Stuff
  uint8_t motor_max_speed;
  uint8_t motor_start_speed;
  int edge_delay;
  int max_additional_delay;
  int minimum_on_time;
  uint8_t pressure_smoothing;
  int sensitivity_threshold;
  int motor_ramp_time_s;
  int update_frequency_hz;
  uint8_t sensor_sensitivity;
  bool use_average_values;

  // Vibration Output Mode
  vibration_mode_t vibration_mode;
  
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

typedef struct config config_t;

extern config_t Config;

// These operations work on the global Config struct. For more lower-level access, check out
// the ones presented on config_defs.h
void config_enqueue_save(long save_at_ms);
bool get_config_value(const char *option, char *buffer, size_t len);
bool set_config_value(const char *option, const char *value, bool *require_reboot);

// TODO: These should be calculated values based on HAL, which works everywhere *BUT*
//       on chart rendering, where we're statically initializing our data array. That
//       should be dynamic initialization, thank yoooou. (Can be classified?)
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

#ifdef __cplusplus
}
#endif

#endif
