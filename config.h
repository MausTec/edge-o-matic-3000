#ifndef __config_h
#define __config_h

#include "arduino.h"

#define CONFIG_FILENAME "/config.json"

// LED Ring Definitions
#define LED_PIN 15
#define LED_COUNT 13

// LCD Definitions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_DC 13
#define OLED_RESET 14
#define OLED_CS 12

// Encoder Connection
#define ENCODER_A_PIN 33
#define ENCODER_B_PIN 32
#define ENCODER_SW_PIN 35
#define ENCODER_RD_PIN 2
#define ENCODER_GR_PIN 0
#define ENCODER_BL_PIN 4

// Buttons
#define KEY_1_PIN 25
#define KEY_2_PIN 26
#define KEY_3_PIN 27

struct ConfigStruct {
  // Networking
  char wifi_ssid[256];
  char wifi_key[256];
  bool wifi_on;

  char bt_display_name[256];
  bool bt_on;

  // UI And Stuff
  byte led_brightness;

  // Server
  int websocket_port;

  // Orgasms and Stuff
  byte motor_max_speed;
} extern Config;
#endif