#include "mbed.h"

// Blinking rate in milliseconds
#define BLINKING_RATE     1000ms
#include "EMW3080BInterface.h"

// Definitions ---------------------------------------------------------

// Change it with your WiFi network name
#define WIFI_NETWORK_NAME       "MA"
// Change it with your WiFi password name
#define WIFI_NETWORK_PASSWORD   "tdmz4879"
#define WIFI_SECURITY           NSAPI_SECURITY_WPA_WPA2

int main()
{
    // Initialise the digital pin LED1 as an output
    DigitalOut led(LED1);

    while (true) {
        printf("Blink!\n");
        led = !led;
        ThisThread::sleep_for(BLINKING_RATE);
    }
}


