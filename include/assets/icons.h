#ifndef __assets__icons_h
#define __assets__icons_h

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ui_icon_def {
    const char icon_state_cnt;
    const unsigned char icon_data[][8];
} ui_icon_def_t;

#define WIFI_ICON_DISCONNECTED 0
#define WIFI_ICON_NO_SIGNAL 1
#define WIFI_ICON_WEAK_SIGNAL 2
#define WIFI_ICON_STRONG_SIGNAL 3
extern const ui_icon_def_t WIFI_ICON;

#define BT_ICON_ACTIVE 0
#define BT_ICON_ISCONNECTED 1
extern const ui_icon_def_t BT_ICON;

#define SD_ICON_PRESENT 0
#define SD_ICON_IN_USE 1
extern const ui_icon_def_t SD_ICON;

#define UPDATE_ICON_PENDING 0
extern const ui_icon_def_t UPDATE_ICON;

#define RECORD_ICON_RECORDING 0
extern const ui_icon_def_t RECORD_ICON;

#ifdef __cplusplus
}
#endif

#endif
