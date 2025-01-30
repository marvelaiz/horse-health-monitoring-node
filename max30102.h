// #ifndef MAX30102_H
// #define MAX30102_H

// #include "mbed.h"

// // Define the MAX30102 I2C address
// #define MAX30102_ADDRESS (0x57 << 1) // Shifted for mbed's 8-bit addressing

// // MAX30102 class
// class MAX30102 {
// public:
//     MAX30102(PinName sda, PinName scl);
//     void initialize();                 // Initialize the sensor
//     void readFIFO(uint32_t &red, uint32_t &ir); // Read Red and IR values
//     void calculateHeartRateAndSpO2(uint32_t red[], uint32_t ir[], int num_samples, 
//                                    float &heartRate, float &spo2); // Calculate HR & SpO2
//     void writeRegister(uint8_t reg, uint8_t value); // Write to a register
//     uint8_t readRegister(uint8_t reg);            // Read from a register

// private:
//     //I2C i2c_external; // I2C instance
// };

// #endif // MAX30102_H