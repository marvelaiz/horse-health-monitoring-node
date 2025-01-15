#include "mbed.h"


// Blinking rate in milliseconds
#define BLINKING_RATE     1000ms
#include "EMW3080BInterface.h"
#include "TCPSocket.h"



#define EMW3080B_WIFI_DEBUG                                         0                                                                                                // set by library:emw3080b
#define EMW3080B_WIFI_FLOW                                          PG_15                                                                                            // set by library:emw3080b[B_U585I_IOT02A]
#define EMW3080B_WIFI_MISO                                          PD_3                                                                                             // set by library:emw3080b[B_U585I_IOT02A]
#define EMW3080B_WIFI_MOSI                                          PD_4                                                                                             // set by library:emw3080b[B_U585I_IOT02A]
#define EMW3080B_WIFI_NOTIFY                                        PD_14                                                                                            // set by library:emw3080b[B_U585I_IOT02A]
#define EMW3080B_WIFI_NSS                                           PB_12                                                                                            // set by library:emw3080b[B_U585I_IOT02A]
#define EMW3080B_WIFI_RESET                                         PF_15                                                                                            // set by library:emw3080b[B_U585I_IOT02A]
#define EMW3080B_WIFI_RX                                            NC                                                                                               // set by library:emw3080b
#define EMW3080B_WIFI_SCLK                                          PD_1                                                                                             // set by library:emw3080b[B_U585I_IOT02A]
#define EMW3080B_WIFI_SPI_INTERFACE                                 1                                                                                                // set by library:emw3080b[B_U585I_IOT02A]
#define EMW3080B_WIFI_TX                                            NC   
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

    int count = 0;

    EMW3080BInterface wifi(EMW3080B_WIFI_DEBUG,
                      EMW3080B_WIFI_MOSI,
                      EMW3080B_WIFI_MISO,
                      EMW3080B_WIFI_SCLK,
                      EMW3080B_WIFI_NSS,
                      EMW3080B_WIFI_NOTIFY,
                      EMW3080B_WIFI_FLOW,
                      EMW3080B_WIFI_RESET,
                      EMW3080B_WIFI_TX,
                      EMW3080B_WIFI_RX,
                      EMW3080B_EMAC::get_instance(),
                      OnboardNetworkStack::get_default_instance()
                     );


    

    while (true) {
        printf("Blink!\n");
        led = !led;
        ThisThread::sleep_for(BLINKING_RATE);
    }
}


