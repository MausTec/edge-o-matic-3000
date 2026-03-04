#include "data_logger.h"
#include "config.h"
#include "eom-hal.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "orgasm_control.h"
#include "orgasm_detection.h"
#include "ui/toast.h"
#include "ui/ui.h"
#include "util/i18n.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char* TAG = "data_logger";

static struct {
    unsigned long recording_start_ms;
    FILE* logfile;
} logger_state;

void data_logger_init(void) {
    logger_state.logfile = NULL;
    logger_state.recording_start_ms = 0;
}

void data_logger_start_recording(void) {
    if (logger_state.logfile) {
        data_logger_stop_recording();
    }

    ui_toast_blocking("%s", _("Preapring recording..."));

    time_t now;
    struct tm timeinfo;
    char filename_date[32];
    time(&now);

    if (!localtime_r(&now, &timeinfo)) {
        ESP_LOGE(TAG, "Failed to obtain time");
        sniprintf(filename_date, 32, "%lld", (esp_timer_get_time() / 1000UL));
    } else {
        strftime(filename_date, 32, "%Y%m%d-%H%M%S", &timeinfo);
    }

    char* logfile_name = NULL;
    asiprintf(&logfile_name, "%s/log-%s.csv", eom_hal_get_sd_mount_point(), filename_date);

    if (!logfile_name) {
        ESP_LOGE(TAG, "Logfile filename buffer issues.");
        ui_toast("%s", _("Error opening logfile!"));
        return;
    }

    ESP_LOGI(TAG, "Opening logfile: %s", logfile_name);
    logger_state.logfile = fopen(logfile_name, "w+");

    if (!logger_state.logfile) {
        ESP_LOGE(TAG, "Couldn't open logfile to save! (%s)", logfile_name);
        ui_toast("%s", _("Error opening logfile!"));
    } else {
        logger_state.recording_start_ms = (esp_timer_get_time() / 1000UL);

        fprintf(
            logger_state.logfile,
            "millis,pressure,avg_pressure,arousal,motor_speed,sensitivity_threshold,"
            "detect_state,detect_mode_used,baseline,sustained_ms,peak_count,last_interval_ms\n"
        );

        ui_set_icon(UI_ICON_RECORD, RECORD_ICON_RECORDING);
        char* fntok = basename(logfile_name);
        ui_toast(_("Recording started:\n%s"), fntok);
    }

    free(logfile_name);
}

void data_logger_stop_recording(void) {
    if (logger_state.logfile != NULL) {
        ui_toast_blocking("%s", _("Stopping..."));
        ESP_LOGI(TAG, "Closing logfile.");
        fclose(logger_state.logfile);
        logger_state.logfile = NULL;
        ui_set_icon(UI_ICON_RECORD, -1);
        ui_toast("%s", _("Recording stopped."));
    }
}

bool data_logger_is_recording(void) {
    return logger_state.logfile != NULL;
}

void data_logger_tick(unsigned long tick_ms) {
    // Build CSV row with all readings + detection state
    char data_csv[320];
    snprintf(
        data_csv,
        sizeof(data_csv),
        "%d,%d,%d,%d,%d,%c,%ld,%ld,%d,%d",
        orgasm_control_get_average_pressure(),
        orgasm_control_get_arousal(),
        eom_hal_get_motor_speed(),
        Config.sensitivity_threshold,
        (int)orgasm_detection_get_state(),
        orgasm_detection_last_was_rhythmic() ? 'R' : 'S',
        orgasm_detection_get_baseline(),
        orgasm_detection_get_sustained_ms(),
        orgasm_detection_get_peak_count(),
        orgasm_detection_get_last_interval_ms()
    );

    // Write to SD card logfile (with millis prefix)
    if (logger_state.logfile != NULL) {
        fprintf(
            logger_state.logfile, "%ld,%s\n", tick_ms - logger_state.recording_start_ms, data_csv
        );
    }

    // Write to serial for classic log mode (without millis, for backward compat)
    if (Config.classic_serial) {
        printf("%s\n", data_csv);
    }
}
