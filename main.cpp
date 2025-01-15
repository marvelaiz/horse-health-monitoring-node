#include "mbed.h"


// Blinking rate in milliseconds
#define BLINKING_RATE     1000ms
#include "EMW3080BInterface.h"
#include "TCPSocket.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"



  
// Definitions ---------------------------------------------------------
//WIFI DRIVER

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

//MQTT
#define MQTT_HOST               "srv-iot.diatel.upm.es"
#define MQTT_PORT               8883
#define MQTT_TOPIC              "v1/devices/me/telemetry"
#define MQTT_TOKEN              "zww36ezetbn7wq6b7oro"  //Send it throught username

// Change it with your WiFi network name
#define WIFI_NETWORK_NAME       "MA"
// Change it with your WiFi password name
#define WIFI_NETWORK_PASSWORD   "tdmz4879"
#define WIFI_SECURITY           NSAPI_SECURITY_WPA_WPA2

SocketAddress sockAddr;
//Adapter class

// Network adapter for Paho MQTT library -------------------------------

class MQTTNetwork
{
public:
    MQTTNetwork(NetworkInterface* aNetwork) : network(aNetwork) {
        socket = new TCPSocket();
        p_sockAddr = new SocketAddress();
    }

    ~MQTTNetwork() {
        delete socket;
    }

    int read(unsigned char* buffer, int len, int timeout) {
        return socket->recv(buffer, len);
    }

    int write(unsigned char* buffer, int len, int timeout) {
        return socket->send(buffer, len);
    }

    int set_TCP_SOCKET(unsigned char* buffer, int len, int timeout) {
        return socket->send(buffer, len);
    }

    int connect(const char* hostname, int port) {
        socket->open(network);
        network->get_ip_address(p_sockAddr);
        network->gethostbyname(hostname, p_sockAddr);
p_sockAddr->set_port(port);
        return socket->connect(*p_sockAddr);
    }

    int disconnect() {
        return socket->close();
    }

private:
    NetworkInterface* network; //wifi handler
    TCPSocket* socket;       //TCP Socket (thingsboard)
    SocketAddress* p_sockAddr;
};


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


    // Scanning WiFi networks ------------------------------------------

    WiFiAccessPoint *ap;

    count = wifi.scan(NULL, 0);
    printf("%d networks available.\n", count);

    /* Limit number of network arbitrary to 15 */
    count = count < 15 ? count : 15;

    ap = new WiFiAccessPoint[count];
    count = wifi.scan(ap, count);
    for (int i = 0; i < count; i++) {
        printf("Network: %s RSSI: %hhd\n", ap[i].get_ssid(), ap[i].get_rssi());
    }

    delete[] ap;

    // Connecting to WiFi network --------------------------------------

    printf("\nConnecting to %s...\n", WIFI_NETWORK_NAME);
    int ret = wifi.connect(WIFI_NETWORK_NAME, WIFI_NETWORK_PASSWORD, WIFI_SECURITY);
    if (ret != 0) {
        printf("\nConnection error\n");
        return -1;
    }

    // Show the network address

    wifi.get_ip_address(&sockAddr);

    printf("Success\n\n");
    printf("MAC: %s\n", wifi.get_mac_address());
 printf("IP: %s\n", sockAddr.get_ip_address());




    // printf("Netmask: %s\n", wifi.get_netmask());
    // printf("Gateway: %s\n", wifi.get_gateway());
    // printf("RSSI: %d\n\n", wifi.get_rssi());


//     // Connect to ThingsBoard ----------------------------------------------

MQTTNetwork network(&wifi);
MQTT::Client<MQTTNetwork, Countdown> client(network);

char assess_token[] = "zww36ezetbn7wq6b7oro";

MQTTPacket_connectData conn_data = MQTTPacket_connectData_initializer;
conn_data.username.cstring = assess_token;

if (network.connect(MQTT_HOST, MQTT_PORT) < 0) {
    printf("failed to connect to " MQTT_HOST  "\n");
    return -1;
}

if (client.connect(conn_data) < 0) {
    printf("failed to send MQTT connect message\n");
    return -1;
}

printf("successfully connect!\n");

client.disconnect();
wifi.disconnect();

printf("\ndone\n");

    while (true) {
        printf("Blink!\n");
        led = !led;
        ThisThread::sleep_for(BLINKING_RATE);
    }
}


