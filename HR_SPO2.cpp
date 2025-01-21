#include "HR_SPO2.h"
#include <math.h>

// Data Buffers
uint32_t red_buffer[BUFFER_SIZE] = {0};
uint32_t ir_buffer[BUFFER_SIZE] = {0};

// Variables for HR and SpO2
float heart_rate = 0.0;
float spo2 = 0.0;

// Function to update data buffers
void update_buffers(uint32_t red_val, uint32_t ir_val) {
    // Shift left and add new value at the end
    for (int i = 0; i < BUFFER_SIZE - 1; i++) {
        red_buffer[i] = red_buffer[i + 1];
        ir_buffer[i] = ir_buffer[i + 1];
    }
    red_buffer[BUFFER_SIZE - 1] = red_val;
    ir_buffer[BUFFER_SIZE - 1] = ir_val;
}

// Function to calculate heart rate
float calculate_heart_rate(const uint32_t ir_signal[BUFFER_SIZE]) {
    int peaks[BUFFER_SIZE];  // Array to store peak indices
    int num_peaks = 0;
    int threshold = 0;
    int peak_interval_sum = 0;
    int peak_interval_count = 0;

    // Find threshold based on max value
    int max_value = ir_signal[0];
    for (int i = 1; i < BUFFER_SIZE; i++) {
        if (ir_signal[i] > max_value) {
            max_value = ir_signal[i];
        }
    }
    threshold = (int)(max_value * 0.6);  // Example threshold

    // Find peaks
    for (int i = 1; i < BUFFER_SIZE - 1; i++) {
        if (ir_signal[i] > threshold && ir_signal[i] > ir_signal[i - 1] && ir_signal[i] > ir_signal[i + 1]) {
            peaks[num_peaks++] = i;
        }
    }

    // Calculate intervals between peaks
    for (int i = 1; i < num_peaks; i++) {
        peak_interval_sum += (peaks[i] - peaks[i - 1]);
        peak_interval_count++;
    }

    if (peak_interval_count == 0) {
        return 0.0;  // Not enough peaks to calculate HR
    }

    float avg_interval = peak_interval_sum / (float)peak_interval_count;
    return 60.0 / (avg_interval / SAMPLING_RATE);  // HR in bpm
}

// Function to calculate SpO2
float calculate_spo2(const uint32_t red_signal[BUFFER_SIZE], const uint32_t ir_signal[BUFFER_SIZE]) {
    float red_ac = 0.0, red_dc = 0.0, ir_ac = 0.0, ir_dc = 0.0;

    // Calculate DC (mean value)
    for (int i = 0; i < BUFFER_SIZE; i++) {
        red_dc += red_signal[i];
        ir_dc += ir_signal[i];
    }
    red_dc /= BUFFER_SIZE;
    ir_dc /= BUFFER_SIZE;

    // Calculate AC (peak-to-peak value)
    int red_max = red_signal[0], red_min = red_signal[0];
    int ir_max = ir_signal[0], ir_min = ir_signal[0];
    for (int i = 1; i < BUFFER_SIZE; i++) {
        if (red_signal[i] > red_max) red_max = red_signal[i];
        if (red_signal[i] < red_min) red_min = red_signal[i];
        if (ir_signal[i] > ir_max) ir_max = ir_signal[i];
        if (ir_signal[i] < ir_min) ir_min = ir_signal[i];
    }

    red_ac = red_max - red_min;
    ir_ac = ir_max - ir_min;

    // Ratio of Ratios
    float ratio = (red_ac / red_dc) / (ir_ac / ir_dc);
    return 110.0 - 25.0 * ratio;  // Example calibration curve
}