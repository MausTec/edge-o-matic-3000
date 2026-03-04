#ifndef __orgasm_detection_h
#define __orgasm_detection_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * Experimental orgasm detection via pelvic floor muscle contractions.
 *
 * This is based on the observation that orgasmic contractions are involuntary, rhythmic,
 * and happen at a remarkably consistent ~0.8 second interval (~1.25 Hz). That number
 * comes from Masters & Johnson's "Human Sexual Response" (1966), which first described
 * the rhythmic contraction pattern, and was later precisely measured by Bohlen et al.
 * in "The Female Orgasm: Pelvic Contractions" (Archives of Sexual Behavior, 1982) using
 * rectal and vaginal pressure transducers — basically what we're doing here, just with
 * fancier lab equipment and more paperwork.
 *
 * The whole idea is that voluntary clenches are either sustained holds or irregularly
 * timed pulses, but the real thing has that distinctive ~0.8s periodicity you can't
 * fake. So we look for that. This is still experimental — we're shipping it to collect
 * real-world data and see how well it actually works outside a lab.
 *
 * By default, the system tries to detect the rhythmic pattern first, but if it doesn't see
 * it, it falls back to a more basic sustained pressure detection method. You can also
 * force it to only look for rhythmic peaks or just sustained pressure if you want to test
 * specific features.
 *
 * Users who wish to contribute to this investigation are encouraged to review the various
 * settings available in the "Orgasm Settings" menu, which allow you to adjust thresholds, timing
 * windows, and other parameters of the detection algorithm. The data recording module SHOULD log
 * and record the detection state and relevant metrics.
 */

/**
 * Orgasm detection state machine.
 *
 * IDLE:      Baseline tracking, waiting for sustained pressure rise.
 * SUSTAINED: Pressure elevated above baseline. Counting duration, detecting peaks.
 * RHYTHMIC:  Rhythmic peaks detected at orgasmic cadence (~0.8 s intervals).
 * DETECTED:  Orgasm detected. Waiting for pressure to return to baseline.
 */
typedef enum {
    OD_STATE_IDLE = 0,
    OD_STATE_SUSTAINED = 1,
    OD_STATE_RHYTHMIC = 2,
    OD_STATE_DETECTED = 3,
} orgasm_detect_state_t;

/**
 * @brief Initialize the orgasm detection module. Call once at startup.
 */
void orgasm_detection_init(void);

/**
 * @brief Run one detection tick. Call once per orgasm_control tick, after updateArousal().
 * @param pressure  Current pressure reading (raw or averaged, same as p_check).
 * @param arousal   Current arousal accumulator value.
 */
void orgasm_detection_tick(uint16_t pressure, uint16_t arousal);

// State getters for UI, logging, and API, etc. Future BLE client; read me!

orgasm_detect_state_t orgasm_detection_get_state(void);
const char* orgasm_detection_get_state_str(void);
uint8_t orgasm_detection_get_peak_count(void);
long orgasm_detection_get_sustained_ms(void);
long orgasm_detection_get_baseline(void);
uint16_t orgasm_detection_get_last_interval_ms(void);
bool orgasm_detection_last_was_rhythmic(void);

#ifdef __cplusplus
}
#endif

#endif
