#ifndef __data_logger_h
#define __data_logger_h

#ifdef __cplusplus
extern "C" {
#endif

#include "orgasm_control.h"
#include <stdbool.h>

/**
 * Data logger module — handles CSV recording to SD card and classic serial output.
 * Extracted from orgasm_control.c to decouple logging from control logic.
 */

void data_logger_init(void);

/**
 * @brief Called once per orgasm_control tick to write CSV data.
 * @param millis_offset  Milliseconds since recording started (for CSV timestamp column).
 */
void data_logger_tick(unsigned long millis_offset);

void data_logger_start_recording(void);
void data_logger_stop_recording(void);
bool data_logger_is_recording(void);

#ifdef __cplusplus
}
#endif

#endif
