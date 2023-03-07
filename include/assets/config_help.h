// clang-format off

#ifndef __assets__config_help_h
#define __assets__config_help_h

#ifdef __cplusplus
extern "C" {
#endif

#define INCLUDE_HELP 1

#ifdef INCLUDE_HELP
#define _HELPSTR(str) str
#else
#define _HELPSTR(str) NULL
#endif

#define WIFI_SSID_HELP _HELPSTR("Your WiFi SSID")
#define WIFI_KEY_HELP _HELPSTR("Your WiFi Password.")
#define WIFI_ON_HELP _HELPSTR("True to enable WiFi / Websocket server.")
#define BT_DISPLAY_NAME_HELP _HELPSTR("AzureFang* device name, you might wanna change this.")
#define BT_ON_HELP _HELPSTR("True to enable the AzureFang connection.")
#define FORCE_BT_COEX_HELP _HELPSTR("True to force AzureFang and WiFi at the same time**.")
#define LED_BRIGHTNESS_HELP _HELPSTR("LED Ring max brightness, only for NoGasm+.")
#define WEBSOCKET_PORT_HELP _HELPSTR("Port to listen for incoming Websocket connections.")
#define USE_SSL_HELP _HELPSTR("Enable SSL server, which will eat all your RAM!")
#define HOSTNAME_HELP _HELPSTR("Local hostname for your device.")
#define MOTOR_START_SPEED_HELP _HELPSTR("The minimum speed the motor will start at in automatic mode.")
#define MOTOR_MAX_SPEED_HELP _HELPSTR("Maximum speed for the motor in auto-ramp mode.")
#define MOTOR_RAMP_TIME_S_HELP _HELPSTR("The time it takes for the motor to reach `motor_max_speed` in auto ramp mode.")
#define EDGE_DELAY_HELP _HELPSTR("Minimum time (ms) after edge detection before resuming stimulation.")
#define MAX_ADDITIONAL_DELAY_HELP _HELPSTR("Maximum time (ms) that can be added to the edge delay before resuming stimulation. A random number will be picked between 0 and this setting each cycle. 0 to disable.")
#define MINIMUM_ON_TIME_HELP _HELPSTR("Time (ms) after stimulation starts before edge detection is resumed.")
#define SCREEN_DIM_SECONDS_HELP _HELPSTR("Time, in seconds, before the screen dims. 0 to disable.")
#define SCREEN_TIMEOUT_SECONDS_HELP _HELPSTR("Time, in seconds, before the screen turns off. 0 to disable.")
#define PRESSURE_SMOOTHING_HELP _HELPSTR("Number of samples to take an average of. Higher results in lag and lower resolution!")
#define CLASSIC_SERIAL_HELP _HELPSTR("Output continuous stream of arousal data over serial for backwards compatibility with other software.")
#define SENSITIVITY_THRESHOLD_HELP _HELPSTR("The arousal threshold for orgasm detection. Lower = sooner cutoff.")
#define UPDATE_FREQUENCY_HZ_HELP _HELPSTR("Update frequency for pressure readings and arousal steps. Higher = crash your serial monitor.")
#define SENSOR_SENSITIVITY_HELP _HELPSTR("Analog pressure prescaling. Adjust this until the pressure is ~60-70%")
#define USE_AVERAGE_VALUES_HELP _HELPSTR("Use average values when calculating arousal. This smooths noisy data.")
#define VIBRATION_MODE_HELP _HELPSTR("Vibration Mode for main vibrator control.")
#define CLENCH_PRESSURE_SENSITIVITY_HELP _HELPSTR("Threshold over arousal to detect a clench : Lower values increase sensitivity")
#define CLENCH_THRESHOLD_2_ORGASM_HELP _HELPSTR("Threshold variable that is tick counts of clench to detect orgasm")
#define CLENCH_DETECTOR_IN_EDGING_HELP _HELPSTR("Use the clench detector to adjust Arousal")
#define AUTO_EDGING_DURATION_MINUTES_HELP _HELPSTR("How long to edge before permiting an orgasm")
#define POST_ORGASM_DURATION_SECONDS_HELP _HELPSTR("How long to stimulate after orgasm detected")
#define EDGE_MENU_LOCK_HELP _HELPSTR("Deny access to menu starting in the edging session")
#define POST_ORGASM_MENU_LOCK_HELP _HELPSTR("Deny access to menu starting after orgasm detected")
#define MAX_CLENCH_DURATION_HELP _HELPSTR("Duration the clench detector can raise arousal if clench detector turned on in edging session")

#ifdef __cplusplus
}
#endif

#endif
