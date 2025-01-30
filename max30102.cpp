// #include "max30102.h"
// #include "mbed.h"


// // I2C Configuration
// I2C i2c_external(PB_9, PB_8);          // SDA, SCL pins (adjust based on your board)

// // Helper function to find the minimum value in an array
// uint32_t findMin(uint32_t arr[], int length) {
//     uint32_t minVal = arr[0];
//     for (int i = 1; i < length; i++) {
//         if (arr[i] < minVal) {
//             minVal = arr[i];
//         }
//     }
//     return minVal;
// }

// // Helper function to find the maximum value in an array
// uint32_t findMax(uint32_t arr[], int length) {
//     uint32_t maxVal = arr[0];
//     for (int i = 1; i < length; i++) {
//         if (arr[i] > maxVal) {
//             maxVal = arr[i];
//         }
//     }
//     return maxVal;
// }

// // Constructor: Initialize I2C
// MAX30102::MAX30102(PinName sda, PinName scl) {
//     i2c_external.frequency(400000); // Set I2C frequency to 400kHz
// }

// // Initialize MAX30102 settings
// void MAX30102::initialize() {
//     printf("Initializing MAX30102...\n");

//     writeRegister(0x09, 0x03); // SpO2 mode (Red and IR LEDs active)
//     writeRegister(0x0A, 0x27); // Set 69us pulse width, 100 samples/sec
//     writeRegister(0x0C, 0x1F); // Red LED current = 6.2mA
//     writeRegister(0x0D, 0x1F); // IR LED current = 6.2mA

//     // Clear FIFO
//     writeRegister(0x04, 0x00); // FIFO_WR_PTR
//     writeRegister(0x05, 0x00); // OVF_COUNTER
//     writeRegister(0x06, 0x00); // FIFO_RD_PTR
// }

// // Write to a MAX30102 register
// void MAX30102::writeRegister(uint8_t reg, uint8_t value) {
//     char data[2] = {static_cast<char>(reg), static_cast<char>(value)};
//     if (i2c_external.write(MAX30102_ADDRESS, data, 2) != 0) {
//         printf("Failed to write to register 0x%02X\n", reg);
//     }
// }

// // Read from a MAX30102 register
// uint8_t MAX30102::readRegister(uint8_t reg) {
//     char data = reg;
//     if (i2c_external.write(MAX30102_ADDRESS, &data, 1) != 0) {
//         printf("Failed to write register address 0x%02X\n", reg);
//         return 0;
//     }

//     char result;
//     if (i2c_external.read(MAX30102_ADDRESS, &result, 1) != 0) {
//         printf("Failed to read from register 0x%02X\n", reg);
//         return 0;
//     }

//     return static_cast<uint8_t>(result);
// }

// // Read Red and IR values from the FIFO
// void MAX30102::readFIFO(uint32_t &red, uint32_t &ir) {
//     char reg = 0x07; // FIFO data register
//     char data[6];

//     if (i2c_external.write(MAX30102_ADDRESS, &reg, 1, true) != 0) {
//         printf("Failed to set FIFO read address.\n");
//         return;
//     }

//     if (i2c_external.read(MAX30102_ADDRESS, data, 6) != 0) {
//         printf("Failed to read FIFO data.\n");
//         return;
//     }

//     // Combine the 3 bytes for Red and IR values
//     red = (data[0] << 16) | (data[1] << 8) | data[2];
//     ir = (data[3] << 16) | (data[4] << 8) | data[5];
// }

// // Calculate heart rate and SpO2
// void MAX30102::calculateHeartRateAndSpO2(uint32_t red[], uint32_t ir[], int num_samples, float &heartRate, float &spo2) {
//     // Variables for SpO2 calculation
//     float ratio, avg_red, avg_ir, sum_red = 0, sum_ir = 0;

//     // Calculate DC components (average)
//     for (int i = 0; i < num_samples; i++) {
//         sum_red += red[i];
//         sum_ir += ir[i];
//     }
//     avg_red = sum_red / num_samples;
//     avg_ir = sum_ir / num_samples;

//     // Calculate AC components (peak-to-peak)
//     uint32_t min_red = findMin(red, num_samples);
// uint32_t max_red = findMax(red, num_samples);
// uint32_t min_ir = findMin(ir, num_samples);
// uint32_t max_ir = findMax(ir, num_samples);

//     float ac_red = max_red - min_red;
//     float ac_ir = max_ir - min_ir;

//     // Calculate ratio
//     ratio = (ac_red / avg_red) / (ac_ir / avg_ir);

//     // Estimate SpO2 using an empirical formula
//     spo2 = 110.0f - 25.0f * ratio;

//     // Heart rate calculation (basic peak detection)
//     int peaks = 0;
//     for (int i = 1; i < num_samples - 1; i++) {
//         if (ir[i] > ir[i - 1] && ir[i] > ir[i + 1]) {
//             peaks++;
//         }
//     }

//     // Calculate heart rate in beats per minute
//     heartRate = (peaks * 60.0f) / (num_samples / 100.0f); // Assuming 100 samples/sec

//      printf("HR: %.2f  & SPO2: %.2f.\n", heartRate, spo2);
// }