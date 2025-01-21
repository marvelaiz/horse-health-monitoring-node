#include "mbed.h"

// Blinking rate in milliseconds
#define BLINKING_RATE 1000ms
#include "EMW3080BInterface.h"
#include "MQTTClient.h"
#include "MQTTPacket.h"
#include "MQTTmbed.h"
#include "TCPSocket.h"
#include "mbed_trace.h"


#define HORSE_INSIDE_FENCING

#ifdef HORSE_INSIDE_FENCING
#define GPS_SIMULATED_LATITUDE_INSIDE_FENCING 40.622
#define GPS_SIMULATED_LONGITUDE_INSIDE_FENCING -3.94

#define GPS_SIMULATED_LATITUDE GPS_SIMULATED_LATITUDE_INSIDE_FENCING
#define GPS_SIMULATED_LONGITUDE GPS_SIMULATED_LONGITUDE_INSIDE_FENCING
#else 
#define  GPS_SIMULATED_LATITUDE_OUTSIDE_FENCING 40.638389
#define  GPS_SIMULATED_LONGITUDE_OUTSIDE_FENCING -3.903117
#define GPS_SIMULATED_LATITUDE GPS_SIMULATED_LATITUDE_OUTSIDE_FENCING
#define GPS_SIMULATED_LONGITUDE GPS_SIMULATED_LONGITUDE_OUTSIDE_FENCING
#endif 


#define MQTT_PUBLISH_FREQ 10000ms
#define MEA_FREQ 5000ms
Ticker trigger_measurements;
Ticker trigger_mqtt_pub_sub;
bool flag_trigger_measurements;
bool flag_tigger_mqtt_pub;

void trigger_measurement_cb() { flag_trigger_measurements = true; }

void trigger_mqtt_pub_cb() { flag_tigger_mqtt_pub = true; }

// I2C Configuration
I2C i2c(PH_5, PH_4);          // SDA, SCL pins (adjust based on your board)
I2C i2c_external(PB_9, PB_8); // SDA, SCL pins (adjust based on your board)
const int ISM330DHCX_I2C_ADDR = 0b1101011 << 1; // 7-bit address shifted for I2C
const int MAX30102_I2C_ADDR = 0x57
                              << 1; // 7-bit I2C address shifted for write/read

#define SAMPLING_RATE 100 // Hz (Adjust according to your setup)
#define BUFFER_SIZE 100   // Number of samples in the buffer

// Data Buffers
// std::vector<uint32_t> red_buffer(BUFFER_SIZE, 0);
// std::vector<uint32_t> ir_buffer(BUFFER_SIZE, 0);

// MAX30102 Register Definitions
#define REG_MODE_CONFIG 0x09
#define REG_SPO2_CONFIG 0x0A
#define REG_LED1_PA 0x0C
#define REG_LED2_PA 0x0D
#define REG_FIFO_WR_PTR 0x04
#define REG_FIFO_RD_PTR 0x06
#define REG_FIFO_DATA 0x07
#define REG_FIFO_OVF_COUNTER 0x05

// Initialization function
void init_max30102() {
  char config[2];

  // Reset the sensor
  config[0] = REG_MODE_CONFIG;
  config[1] = 0x40; // Reset bit
  i2c.write(MAX30102_I2C_ADDR, config, 2);
  ThisThread::sleep_for(100ms);

  // Configure SpO2 mode
  config[0] = REG_MODE_CONFIG;
  config[1] = 0x03; // SpO2 mode
  i2c.write(MAX30102_I2C_ADDR, config, 2);

  // Set LED pulse amplitude for RED and IR
  config[0] = REG_LED1_PA;
  config[1] = 0x24; // RED LED current
  i2c.write(MAX30102_I2C_ADDR, config, 2);

  config[0] = REG_LED2_PA;
  config[1] = 0x24; // IR LED current
  i2c.write(MAX30102_I2C_ADDR, config, 2);

  // Configure FIFO
  config[0] = REG_SPO2_CONFIG;
  config[1] = 0x27; // SpO2 ADC range = 4096 nA, 100 Hz, 18-bit resolution
  i2c.write(MAX30102_I2C_ADDR, config, 2);

  config[0] = REG_FIFO_WR_PTR; // Clear FIFO pointers
  config[1] = 0x00;
  i2c.write(MAX30102_I2C_ADDR, config, 2);

  config[0] = REG_FIFO_RD_PTR;
  config[1] = 0x00;
  i2c.write(MAX30102_I2C_ADDR, config, 2);

  config[0] = REG_FIFO_OVF_COUNTER;
  config[1] = 0x00;
  i2c.write(MAX30102_I2C_ADDR, config, 2);
}

// Function to read heart rate and SpO2
void read_heart_rate_and_spo2(float &heart_rate, float &spo2) {
  char reg = REG_FIFO_DATA;
  char data[6];
  uint32_t red_val = 0, ir_val = 0;

  // Read 6 bytes from the FIFO data register
  i2c.write(MAX30102_I2C_ADDR, &reg, 1, true);
  i2c.read(MAX30102_I2C_ADDR, data, 6);

  // Combine the 3 bytes for each channel
  red_val = (data[0] << 16) | (data[1] << 8) | data[2];
  ir_val = (data[3] << 16) | (data[4] << 8) | data[5];

  // Placeholder algorithm to calculate bpm and SpO2 from raw data
  // In real implementations, you would use a proper algorithm/library
  heart_rate = (ir_val % 100) + 60; // Mock calculation
  spo2 = (red_val % 10) + 95;       // Mock calculation
}

// Register Addresses
#define CTRL1_XL 0x10 // Control register for accelerometer
#define OUTX_L_A 0x28 // Acceleration data registers
#define OUTX_H_A 0x29
#define OUTY_L_A 0x2A
#define OUTY_H_A 0x2B
#define OUTZ_L_A 0x2C
#define OUTZ_H_A 0x2D

// Initialization function
void init_accelerometer() {
  char data[2];

  // Set accelerometer to high-performance mode with ODR = 104 Hz, FS = ±2g
  data[0] = CTRL1_XL;
  data[1] = 0x60; // 0b01100000: ODR = 104 Hz, FS = ±2g, LPF enabled
  i2c.write(ISM330DHCX_I2C_ADDR, data, 2);
}

// Function to read accelerometer data
void read_acceleration(float &x, float &y, float &z) {
  char reg = OUTX_L_A;
  char data[6];
  int16_t raw_x, raw_y, raw_z;

  // Request acceleration data
  i2c.write(ISM330DHCX_I2C_ADDR, &reg, 1, true);
  i2c.read(ISM330DHCX_I2C_ADDR, data, 6);

  // Convert raw data
  raw_x = (data[1] << 8) | data[0];
  raw_y = (data[3] << 8) | data[2];
  raw_z = (data[5] << 8) | data[4];

  // Convert to g using sensitivity for ±2g (0.061 mg/LSB)
  x = raw_x * 0.061 / 1000.0f;
  y = raw_y * 0.061 / 1000.0f;
  z = raw_z * 0.061 / 1000.0f;
}

static const char cert[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEMjCCAxqgAwIBAgIBATANBgkqhkiG9w0BAQUFADB7MQswCQYDVQQGEwJHQjEb\n"
    "MBkGA1UECAwSR3JlYXRlciBNYW5jaGVzdGVyMRAwDgYDVQQHDAdTYWxmb3JkMRow\n"
    "GAYDVQQKDBFDb21vZG8gQ0EgTGltaXRlZDEhMB8GA1UEAwwYQUFBIENlcnRpZmlj\n"
    "YXRlIFNlcnZpY2VzMB4XDTA0MDEwMTAwMDAwMFoXDTI4MTIzMTIzNTk1OVowezEL\n"
    "MAkGA1UEBhMCR0IxGzAZBgNVBAgMEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4GA1UE\n"
    "BwwHU2FsZm9yZDEaMBgGA1UECgwRQ29tb2RvIENBIExpbWl0ZWQxITAfBgNVBAMM\n"
    "GEFBQSBDZXJ0aWZpY2F0ZSBTZXJ2aWNlczCCASIwDQYJKoZIhvcNAQEBBQADggEP\n"
    "ADCCAQoCggEBAL5AnfRu4ep2hxxNRUSOvkbIgwadwSr+GB+O5AL686tdUIoWMQua\n"
    "BtDFcCLNSS1UY8y2bmhGC1Pqy0wkwLxyTurxFa70VJoSCsN6sjNg4tqJVfMiWPPe\n"
    "3M/vg4aijJRPn2jymJBGhCfHdr/jzDUsi14HZGWCwEiwqJH5YZ92IFCokcdmtet4\n"
    "YgNW8IoaE+oxox6gmf049vYnMlhvB/VruPsUK6+3qszWY19zjNoFmag4qMsXeDZR\n"
    "rOme9Hg6jc8P2ULimAyrL58OAd7vn5lJ8S3frHRNG5i1R8XlKdH5kBjHYpy+g8cm\n"
    "ez6KJcfA3Z3mNWgQIJ2P2N7Sw4ScDV7oL8kCAwEAAaOBwDCBvTAdBgNVHQ4EFgQU\n"
    "oBEKIz6W8Qfs4q8p74Klf9AwpLQwDgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQF\n"
    "MAMBAf8wewYDVR0fBHQwcjA4oDagNIYyaHR0cDovL2NybC5jb21vZG9jYS5jb20v\n"
    "QUFBQ2VydGlmaWNhdGVTZXJ2aWNlcy5jcmwwNqA0oDKGMGh0dHA6Ly9jcmwuY29t\n"
    "b2RvLm5ldC9BQUFDZXJ0aWZpY2F0ZVNlcnZpY2VzLmNybDANBgkqhkiG9w0BAQUF\n"
    "AAOCAQEACFb8AvCb6P+k+tZ7xkSAzk/ExfYAWMymtrwUSWgEdujm7l3sAg9g1o1Q\n"
    "GE8mTgHj5rCl7r+8dFRBv/38ErjHT1r0iWAFf2C3BUrz9vHCv8S5dIa2LX1rzNLz\n"
    "Rt0vxuBqw8M0Ayx9lt1awg6nCpnBBYurDC/zXDrPbDdVCYfeU0BsWO/8tqtlbgT2\n"
    "G9w84FoVxp7Z8VlIMCFlA2zs6SFz7JsDoeA3raAVGI/6ugLOpyypEBMs1OUIJqsi\n"
    "l2D4kF501KKaU73yqWjgom7C12yxow+ev+to51byrvLjKzg6CYG1a4XXvi3tPxq3\n"
    "smPi9WIsgtRqAEFQ8TmDn5XpNpaYbg==\n"
    "-----END CERTIFICATE-----\n";

// Definitions ---------------------------------------------------------
// WIFI DRIVER

#define EMW3080B_WIFI_DEBUG 0         // set by library:emw3080b
#define EMW3080B_WIFI_FLOW PG_15      // set by library:emw3080b[B_U585I_IOT02A]
#define EMW3080B_WIFI_MISO PD_3       // set by library:emw3080b[B_U585I_IOT02A]
#define EMW3080B_WIFI_MOSI PD_4       // set by library:emw3080b[B_U585I_IOT02A]
#define EMW3080B_WIFI_NOTIFY PD_14    // set by library:emw3080b[B_U585I_IOT02A]
#define EMW3080B_WIFI_NSS PB_12       // set by library:emw3080b[B_U585I_IOT02A]
#define EMW3080B_WIFI_RESET PF_15     // set by library:emw3080b[B_U585I_IOT02A]
#define EMW3080B_WIFI_RX NC           // set by library:emw3080b
#define EMW3080B_WIFI_SCLK PD_1       // set by library:emw3080b[B_U585I_IOT02A]
#define EMW3080B_WIFI_SPI_INTERFACE 1 // set by library:emw3080b[B_U585I_IOT02A]
#define EMW3080B_WIFI_TX NC

// MQTT
#define MQTT_HOST "srv-iot.diatel.upm.es"
#define MQTT_PORT 8883
// #define MQTT_HOST               "test.mosquitto.org"
// #define MQTT_PORT               1883

#define MAX_MQTT_PACKET_SIZE 1000

#define MQTT_TOPIC "v1/devices/me/telemetry"
#define MQTT_TOKEN "zww36ezetbn7wq6b7oro" // Send it throught username

// Change it with your WiFi network name
#define WIFI_NETWORK_NAME "MA"
// Change it with your WiFi password name
#define WIFI_NETWORK_PASSWORD "tdmz4879"
#define WIFI_SECURITY NSAPI_SECURITY_WPA_WPA2

SocketAddress sockAddr;
// Adapter class

// Network adapter for Paho MQTT library -------------------------------

class MQTTNetwork {
public:
  MQTTNetwork(NetworkInterface *aNetwork) : network(aNetwork) {
    socket = new TCPSocket();
    p_sockAddr = new SocketAddress();
    tls_socket = new TLSSocket;
  }

  ~MQTTNetwork() { delete socket; }

  int read(unsigned char *buffer, int len, int timeout) {
    // return socket->recv(buffer, len);
    return tls_socket->recv(buffer, len);
  }

  int write(unsigned char *buffer, int len, int timeout) {
    // return socket->send(buffer, len);
    return tls_socket->send(buffer, len);
  }

  int set_TCP_SOCKET(unsigned char *buffer, int len, int timeout) {
    // return socket->send(buffer, len);
    return tls_socket->send(buffer, len);
  }

  int connect(const char *hostname, int port, OnboardNetworkStack &stack) {

    result = tls_socket->set_root_ca_cert(cert);
    if (result != 0) {
      printf("Error: socket->set_root_ca_cert() returned %d\n", result);
      return result;
    }
    result = tls_socket->open(&stack);
    if (result != 0) {
      printf("Error! socket->open() returned: %d\n", result);
      return result;
    }

    network->gethostbyname(MQTT_HOST, p_sockAddr);

    p_sockAddr->set_port(MQTT_PORT);
    return tls_socket->connect(*p_sockAddr);

    // if (result != 0) {
    //   printf("TLS NOT  %d\n", result);
    // } else {
    //   printf("TLS OK");
    // }
  }

  int disconnect() {
    // return  tls->close();
    return socket->close();
  }

  NetworkInterface *network; // wifi handler
  TCPSocket *socket;         // TCP Socket (thingsboard)
  SocketAddress *p_sockAddr;
  //   TLSSocketWrapper *tls; // Wrapper of TCP socket over TLS
  TLSSocket *tls_socket;
  nsapi_size_or_error_t result;
};

void init_tickers() {
  flag_tigger_mqtt_pub = true;
  flag_trigger_measurements = true;
  trigger_measurements.attach(
      &trigger_measurement_cb,
      MEA_FREQ); // the address of the function to be attached (flip) and the
                 // interval (2 seconds)
  trigger_measurements.attach(
      &trigger_measurement_cb,
      MQTT_PUBLISH_FREQ); // the address of the function to be attached (flip)
                          // and the interval (2 seconds)
}

int main() {

  

  // INIT SENSORS

  // Initialize the accelerometer
  init_accelerometer();

  // Initialize the MAX30102
  init_max30102();

  // float heart_rate, spo2;

  nsapi_size_or_error_t result;
  // Initialise the digital pin LED1 as an output
  DigitalOut led(LED1);

  int count = 0;
  OnboardNetworkStack &stack = OnboardNetworkStack::get_default_instance();

  EMW3080BInterface wifi(
      EMW3080B_WIFI_DEBUG, EMW3080B_WIFI_MOSI, EMW3080B_WIFI_MISO,
      EMW3080B_WIFI_SCLK, EMW3080B_WIFI_NSS, EMW3080B_WIFI_NOTIFY,
      EMW3080B_WIFI_FLOW, EMW3080B_WIFI_RESET, EMW3080B_WIFI_TX,
      EMW3080B_WIFI_RX, EMW3080B_EMAC::get_instance(), stack);

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
  int ret =
      wifi.connect(WIFI_NETWORK_NAME, WIFI_NETWORK_PASSWORD, WIFI_SECURITY);
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
  //     // Connect to ThingsBoard
  //     ----------------------------------------------

  MQTTNetwork network(&wifi);

  MQTT::Client<MQTTNetwork, Countdown> client(network);

  char assess_token[] = "zww36ezetbn7wq6b7oro";
  MQTTPacket_connectData conn_data = MQTTPacket_connectData_initializer;
  conn_data.username.cstring = (char *)MQTT_TOKEN;
  //   conn_data.username.lenstring.len = 20;
  // //   conn_data.username.lenstring.data = (char *)MQTT_TOKEN;
  //   conn_data.MQTTVersion = 3;

  //   if (network.connect(MQTT_HOST, MQTT_PORT) < 0) {
  //       printf("TCP with TLS: failed to connect to " MQTT_HOST  "\n");
  //       return -1;
  //   }else{
  //       printf("TCP with TLS: Connected to " MQTT_HOST  "\n");
  //   }
  result = network.connect(MQTT_HOST, MQTT_PORT, stack);
  if (result < 0) {
    printf("TCP with TLS: failed to connect to " MQTT_HOST " %d\n", result);
    return -1;
  } else {
    printf("TCP with TLS: Connected to " MQTT_HOST "\n");
  }

  //   TLSSocket *socket = new TLSSocket; //

  //   result = socket->set_root_ca_cert(cert);

  //   if (result != 0) {
  //     printf("Error: socket->set_root_ca_cert() returned %d\n", result);
  //     return result;
  //   }

  //   result = socket->open(&stack);
  //   if (result != 0) {
  //     printf("Error! socket->open() returned: %d\n", result);
  //     return result;
  //   }

  //   SocketAddress *p_sockAddr = new SocketAddress();
  //   wifi.gethostbyname(MQTT_HOST, p_sockAddr);

  //   p_sockAddr->set_port(MQTT_PORT);
  //   result = socket->connect(*p_sockAddr);

  //   if (result != 0) {
  //     printf("TLS NOT  %d\n", result);
  //   } else {
  //     printf("TLS OK");
  //   }

  result = client.connect(conn_data);

  if (result != 0) {
    printf("Connected  NOT OK %d\n", result);
  } else {
    printf("Connected OK");
  }

  char msg[1000];
  // Define the fields and their corresponding values
  float temperature = 38.2;
  int hr = 80;
  float spo2 = 97.5;
  float x = 10, y = 20, z = 30;

  

  float lat = GPS_SIMULATED_LATITUDE, lon = GPS_SIMULATED_LONGITUDE;

  // result=-1;
  // while(result==-1){
  //     result = client.connect(conn_data);

  //   if (result < 0) {
  //     printf("failed to send MQTT connect message . %d\n", result);
  //     client.disconnect();
  //     ThisThread::sleep_for(2000ms);
  //     // return -1;
  //   } else {
  //     printf("successfully connect!\n");
  //   }
  //
  //
  // }

  // if (client.connect(conn_data) < 0) {
  //     printf("failed to send MQTT connect message\n");
  //     // return -1;
  // }else{
  //     printf("successfully connect!\n");
  // }

  // client.disconnect();
  // wifi.disconnect();
  printf("\ndone\n");

  init_tickers();

  while (true) {

    // if (flag_trigger_measurements) {
        flag_trigger_measurements=false;
      // 3D Gyroscope

      // Create the JSON message using snprintf
      read_acceleration(x, y, z);
      // Read heart rate and SpO2
      // read_heart_rate_and_spo2(hr, spo2);
      x = x * 9.8;
      y = y * 9.8;
      z = z * 9.8;
    // }

    // if(flag_trigger_measurements){
        flag_trigger_measurements=false;

        int n = snprintf(msg, sizeof(msg),
                     "{\"temperature\":%f,\"HR\":%d,\"oximetry\":%f,\"x\":"
                     "%.2f,\"y\":%.2f,\"z\":%.2f,\"lat\":%f,\"long\":%f}",
                     temperature, hr, spo2, x, y, z, lat, lon);

    void *payload = reinterpret_cast<void *>(msg);
    size_t payload_len = n;

    printf("Msg published to: %s %d %s\r\n", MQTT_HOST, MQTT_PORT, MQTT_TOPIC);

    if (client.publish(MQTT_TOPIC, payload, n) < 0) {
      printf("failed to publish MQTT message");
    }
    led = !led;

    // }
    
    ThisThread::sleep_for(BLINKING_RATE);
  }
}