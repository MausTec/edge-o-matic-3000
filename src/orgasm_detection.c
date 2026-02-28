#include "orgasm_detection.h"
#include "config.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "system/event_manager.h"
#include <math.h>
#include <string.h>

static const char* TAG = "orgasm_detection";

// Ring buffer for peak timestamps and amplitudes
#define PEAK_RING_SIZE 8

typedef struct {
    unsigned long timestamp_ms;
    uint16_t amplitude; // peak-to-trough excursion
} peak_record_t;

static struct {
    orgasm_detect_state_t state;

    // Baseline tracking (slow EMA, ~5 s time constant)
    float baseline;
    bool baseline_initialized;

    // Sustained phase timing
    unsigned long sustained_start_ms;
    unsigned long last_above_threshold_ms;

    // Peak detection (simple 3-sample comparator)
    uint16_t prev_pressure[3]; // [oldest, mid, newest]
    uint8_t prev_count;        // how many samples we've collected
    uint16_t peak_trough;      // running local minimum for peak amplitude calc

    // Rhythmic peak ring buffer
    peak_record_t peaks[PEAK_RING_SIZE];
    uint8_t peak_write_idx;
    uint8_t peak_count; // total peaks detected in current episode

    // Detection result
    bool last_was_rhythmic;
    unsigned long detected_at_ms;

    // Last computed interval for logging
    uint16_t last_interval_ms;
} det;

// ---- Internal helpers

static unsigned long _now_ms(void) {
    return (unsigned long)(esp_timer_get_time() / 1000UL);
}

/**
 * @brief Update the slow EMA baseline tracker.
 *
 * Time constant is ~5 seconds. For a given tick rate hz:
 *   alpha = 1 - exp(-1 / (tau * hz))
 * At 50 Hz, tau=5: alpha ≈ 0.004
 * 
 * Parts of this algorithm were derived from the Mercury chamber position tracking. Support will not
 * be provided to try and understand this, the full documentation is internal.
 */
static void _update_baseline(uint16_t pressure) {
    if (!det.baseline_initialized) {
        det.baseline = (float)pressure;
        det.baseline_initialized = true;
        return;
    }

    float tau = 5.0f; // seconds
    float alpha = 1.0f - expf(-1.0f / (tau * (float)Config.update_frequency_hz));

    // Only track baseline downward or during IDLE — don't chase sustained clenches upward.
    if (det.state == OD_STATE_IDLE) {
        det.baseline = det.baseline + alpha * ((float)pressure - det.baseline);
    } else {
        // During active detection, only let baseline drift downward (or very slowly up).
        if (pressure < det.baseline) {
            det.baseline = det.baseline + alpha * ((float)pressure - det.baseline);
        }
    }
}

/**
 * @brief Simple 3-sample peak detector. Returns true if prev_pressure[1] is a local maximum
 *        above the minimum amplitude threshold.
 */
static bool _detect_peak(uint16_t* out_amplitude) {
    if (det.prev_count < 3) return false;

    uint16_t a = det.prev_pressure[0];
    uint16_t b = det.prev_pressure[1]; // candidate peak
    uint16_t c = det.prev_pressure[2];

    if (b > a && b >= c) {
        // Local maximum found. Compute amplitude as peak - trough.
        uint16_t amplitude = (b > det.peak_trough) ? (b - det.peak_trough) : 0;

        if (amplitude >= (uint16_t)Config.od_peak_min_amplitude) {
            *out_amplitude = amplitude;
            det.peak_trough = c; // reset trough for next peak
            return true;
        }
    }

    // Track trough (running minimum since last peak)
    if (c < det.peak_trough) {
        det.peak_trough = c;
    }

    return false;
}

/**
 * @brief Record a detected peak into the ring buffer.
 */
static void _record_peak(unsigned long timestamp_ms, uint16_t amplitude) {
    det.peaks[det.peak_write_idx].timestamp_ms = timestamp_ms;
    det.peaks[det.peak_write_idx].amplitude = amplitude;
    det.peak_write_idx = (det.peak_write_idx + 1) % PEAK_RING_SIZE;
    det.peak_count++;
}

/**
 * @brief Check if the last N peaks form a rhythmic pattern within the configured interval bounds.
 * @param min_consecutive  Minimum consecutive peaks that must be in rhythm.
 * @return true if rhythmic pattern detected.
 */
static bool _check_rhythmic(int min_consecutive) {
    if (det.peak_count < (uint8_t)min_consecutive) return false;

    // Walk backward through the ring buffer checking intervals.
    int consecutive = 0;
    float sum_interval = 0;
    float sum_sq_interval = 0;

    int count = det.peak_count < PEAK_RING_SIZE ? det.peak_count : PEAK_RING_SIZE;
    if (count < min_consecutive) return false;

    for (int i = 0; i < count - 1; i++) {
        int idx_newer = (det.peak_write_idx - 1 - i + PEAK_RING_SIZE) % PEAK_RING_SIZE;
        int idx_older = (det.peak_write_idx - 2 - i + PEAK_RING_SIZE) % PEAK_RING_SIZE;

        unsigned long newer = det.peaks[idx_newer].timestamp_ms;
        unsigned long older = det.peaks[idx_older].timestamp_ms;

        if (newer <= older) break; // corrupted or wrapped data

        unsigned long interval = newer - older;
        det.last_interval_ms = (uint16_t)(interval > 0xFFFF ? 0xFFFF : interval);

        if (interval < (unsigned long)Config.od_rhythmic_interval_min_ms ||
            interval > (unsigned long)Config.od_rhythmic_interval_max_ms) {
            break; // out of expected cadence range
        }

        consecutive++;
        sum_interval += (float)interval;
        sum_sq_interval += (float)interval * (float)interval;
    }

    if (consecutive < min_consecutive - 1) return false; // need N peaks = N-1 intervals

    // Check interval variance
    float n = (float)consecutive;
    float mean = sum_interval / n;
    float variance = (sum_sq_interval / n) - (mean * mean);
    float std_dev = sqrtf(variance > 0 ? variance : 0);

    return std_dev <= (float)Config.od_rhythmic_interval_variance_ms;
}

/**
 * @brief Reset detection state to IDLE.
 */
static void _reset_to_idle(void) {
    det.state = OD_STATE_IDLE;
    det.peak_count = 0;
    det.peak_write_idx = 0;
    det.peak_trough = 0xFFFF;
    det.last_interval_ms = 0;
    det.sustained_start_ms = 0;
    det.last_above_threshold_ms = 0;
    memset(det.peaks, 0, sizeof(det.peaks));
}

// ---- Public API

void orgasm_detection_init(void) {
    memset(&det, 0, sizeof(det));
    det.peak_trough = 0xFFFF;
    det.baseline_initialized = false;
    ESP_LOGI(TAG, "Orgasm detection initialized (mode=%d)", Config.od_mode);
}

void orgasm_detection_tick(uint16_t pressure, uint16_t arousal) {
    unsigned long now = _now_ms();

    // Always update baseline
    _update_baseline(pressure);

    // Shift pressure history
    det.prev_pressure[0] = det.prev_pressure[1];
    det.prev_pressure[1] = det.prev_pressure[2];
    det.prev_pressure[2] = pressure;
    if (det.prev_count < 3) det.prev_count++;

    // Pressure relative to baseline
    long pressure_above_baseline = (long)pressure - (long)det.baseline;

    // Arousal gate: check if arousal is high enough to arm detection
    bool arousal_gate_met = true;
    if (Config.od_arousal_gate_percent > 0 && Config.sensitivity_threshold > 0) {
        int gate_level = (Config.sensitivity_threshold * Config.od_arousal_gate_percent) / 100;
        arousal_gate_met = (arousal >= (uint16_t)gate_level);
    }

    orgasm_detect_mode_t mode = (orgasm_detect_mode_t)Config.od_mode;

    switch (det.state) {
    case OD_STATE_IDLE: {
        if (pressure_above_baseline >= Config.od_sustained_threshold && arousal_gate_met) {
            det.state = OD_STATE_SUSTAINED;
            det.sustained_start_ms = now;
            det.last_above_threshold_ms = now;
            det.peak_count = 0;
            det.peak_write_idx = 0;
            det.peak_trough = pressure; // init trough to current value
            det.last_interval_ms = 0;
            memset(det.peaks, 0, sizeof(det.peaks));
            ESP_LOGD(TAG, "IDLE -> SUSTAINED (pressure=%d, baseline=%.0f)", pressure, det.baseline);
        }
        break;
    }

    case OD_STATE_SUSTAINED: {
        // Check for dropout: pressure fell below threshold for too long
        if (pressure_above_baseline >= Config.od_sustained_threshold) {
            det.last_above_threshold_ms = now;
        } else if ((now - det.last_above_threshold_ms) > (unsigned long)Config.od_sustained_dropout_ms) {
            ESP_LOGD(TAG, "SUSTAINED -> IDLE (dropout)");
            _reset_to_idle();
            break;
        }

        // Detect peaks for rhythmic analysis
        uint16_t peak_amplitude = 0;
        if (_detect_peak(&peak_amplitude)) {
            _record_peak(now, peak_amplitude);
            ESP_LOGD(TAG, "Peak #%d detected (amplitude=%d)", det.peak_count, peak_amplitude);

            // Check for rhythmic pattern
            if (mode != OD_MODE_SUSTAINED) {
                if (_check_rhythmic(3)) {
                    det.state = OD_STATE_RHYTHMIC;
                    ESP_LOGD(TAG, "SUSTAINED -> RHYTHMIC");
                    break;
                }
            }
        }

        // Sustained-only fallback (AUTO or SUSTAINED mode)
        if (mode != OD_MODE_RHYTHMIC) {
            unsigned long sustained_ms = now - det.sustained_start_ms;
            if (sustained_ms >= (unsigned long)Config.od_sustained_fallback_ms) {
                // Check arousal gate is still met
                if (arousal_gate_met) {
                    det.state = OD_STATE_DETECTED;
                    det.last_was_rhythmic = false;
                    det.detected_at_ms = now;
                    ESP_LOGI(
                        TAG, "SUSTAINED -> DETECTED (sustained fallback, %lu ms)", sustained_ms
                    );
                    if (Config.od_detection_armed) {
                        event_manager_dispatch(EVT_ORGASM_START, NULL, 0);
                    }
                }
            }
        }

        // Clench arousal boost
        if (Config.od_clench_arousal_boost) {
            // The boost is applied by the caller (orgasm_control) reading our state.
            // We just stay in SUSTAINED — the caller checks our state each tick.
            // We should evaluate if it would make sense to pull this logic into here,
            // but this entire file was a mess to rewrite.
        }

        break;
    }

    case OD_STATE_RHYTHMIC: {
        // Continue detecting peaks
        uint16_t peak_amplitude = 0;
        if (_detect_peak(&peak_amplitude)) {
            _record_peak(now, peak_amplitude);
            ESP_LOGD(TAG, "Rhythmic peak #%d (amplitude=%d)", det.peak_count, peak_amplitude);
        }

        // Check if we've lost rhythmic pattern
        if (det.peak_count > 0) {
            int last_peak_idx = (det.peak_write_idx - 1 + PEAK_RING_SIZE) % PEAK_RING_SIZE;
            unsigned long since_last_peak = now - det.peaks[last_peak_idx].timestamp_ms;

            if (since_last_peak > (unsigned long)Config.od_rhythmic_timeout_ms) {
                // Lost rhythmic pattern — fall back to SUSTAINED, not IDLE
                det.state = OD_STATE_SUSTAINED;
                ESP_LOGD(TAG, "RHYTHMIC -> SUSTAINED (timeout %lu ms)", since_last_peak);
                break;
            }
        }

        // Check for confirmed rhythmic orgasm
        if (det.peak_count >= (uint8_t)Config.od_rhythmic_min_peaks) {
            if (_check_rhythmic(Config.od_rhythmic_min_peaks)) {
                // Halved arousal gate for rhythmic detection — we're more confident
                bool rhythmic_gate_met = true;
                if (Config.od_arousal_gate_percent > 0 && Config.sensitivity_threshold > 0) {
                    int gate_level =
                        (Config.sensitivity_threshold * Config.od_arousal_gate_percent) / 200;
                    rhythmic_gate_met = (arousal >= (uint16_t)gate_level);
                }

                if (rhythmic_gate_met) {
                    det.state = OD_STATE_DETECTED;
                    det.last_was_rhythmic = true;
                    det.detected_at_ms = now;
                    ESP_LOGI(
                        TAG,
                        "RHYTHMIC -> DETECTED (%d peaks, last_interval=%d ms)",
                        det.peak_count,
                        det.last_interval_ms
                    );
                    if (Config.od_detection_armed) {
                        event_manager_dispatch(EVT_ORGASM_START, NULL, 0);
                    }
                }
            }
        }
        break;
    }

    case OD_STATE_DETECTED: {
        // Wait for pressure to return to baseline range for recovery_ms
        bool near_baseline = (pressure_above_baseline < Config.od_sustained_threshold / 2);

        if (near_baseline) {
            if ((now - det.detected_at_ms) >= (unsigned long)Config.od_recovery_ms) {
                ESP_LOGD(TAG, "DETECTED -> IDLE (recovered)");
                _reset_to_idle();
            }
        } else {
            // Pressure still elevated — reset recovery timer
            det.detected_at_ms = now;
        }
        break;
    }

    default: _reset_to_idle(); break;
    }
}

orgasm_detect_state_t orgasm_detection_get_state(void) {
    return det.state;
}

const char* orgasm_detection_get_state_str(void) {
    switch (det.state) {
    case OD_STATE_IDLE: return "IDLE";
    case OD_STATE_SUSTAINED: return "SUSTAINED";
    case OD_STATE_RHYTHMIC: return "RHYTHMIC";
    case OD_STATE_DETECTED: return "DETECTED";
    default: return "UNKNOWN";
    }
}

uint8_t orgasm_detection_get_peak_count(void) {
    return det.peak_count;
}

long orgasm_detection_get_sustained_ms(void) {
    if (det.state == OD_STATE_IDLE || det.sustained_start_ms == 0) return 0;
    return (long)(_now_ms() - det.sustained_start_ms);
}

long orgasm_detection_get_baseline(void) {
    return (long)det.baseline;
}

uint16_t orgasm_detection_get_last_interval_ms(void) {
    return det.last_interval_ms;
}

bool orgasm_detection_last_was_rhythmic(void) {
    return det.last_was_rhythmic;
}
