#ifndef HEART_RATE_MONITOR_H
#define HEART_RATE_MONITOR_H

#include "mbed.h"

// Constants
#define SAMPLING_RATE 100   // Hz
#define BUFFER_SIZE 100     // Number of samples in the buffer

// Data Buffers
extern uint32_t red_buffer[BUFFER_SIZE];
extern uint32_t ir_buffer[BUFFER_SIZE];

// Variables for HR and SpO2
extern float heart_rate;
extern float spo2;

// Function declarations
void update_buffers(uint32_t red_val, uint32_t ir_val);
float calculate_heart_rate(const uint32_t ir_signal[BUFFER_SIZE]);
float calculate_spo2(const uint32_t red_signal[BUFFER_SIZE], const uint32_t ir_signal[BUFFER_SIZE]);

#endif // HEART_RATE_MONITOR_H