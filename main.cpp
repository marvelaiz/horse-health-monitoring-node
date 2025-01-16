#include "mbed.h"

// Blinking rate in milliseconds
#define BLINKING_RATE 1000ms
#include "EMW3080BInterface.h"
#include "MQTTClient.h"
#include "MQTTPacket.h"
#include "MQTTmbed.h"
#include "TCPSocket.h"
#include "mbed_trace.h"

// static const char cert[] =  "-----BEGIN CERTIFICATE-----\n"
//     "MIIEAzCCAuugAwIBAgIUBY1hlCGvdj4NhBXkZ/uLUZNILAwwDQYJKoZIhvcNAQEL\n"
//     "BQAwgZAxCzAJBgNVBAYTAkdCMRcwFQYDVQQIDA5Vbml0ZWQgS2luZ2RvbTEOMAwG\n"
//     "A1UEBwwFRGVyYnkxEjAQBgNVBAoMCU1vc3F1aXR0bzELMAkGA1UECwwCQ0ExFjAU\n"
//     "BgNVBAMMDW1vc3F1aXR0by5vcmcxHzAdBgkqhkiG9w0BCQEWEHJvZ2VyQGF0Y2hv\n"
//     "by5vcmcwHhcNMjAwNjA5MTEwNjM5WhcNMzAwNjA3MTEwNjM5WjCBkDELMAkGA1UE\n"
//     "BhMCR0IxFzAVBgNVBAgMDlVuaXRlZCBLaW5nZG9tMQ4wDAYDVQQHDAVEZXJieTES\n"
//     "MBAGA1UECgwJTW9zcXVpdHRvMQswCQYDVQQLDAJDQTEWMBQGA1UEAwwNbW9zcXVp\n"
//     "dHRvLm9yZzEfMB0GCSqGSIb3DQEJARYQcm9nZXJAYXRjaG9vLm9yZzCCASIwDQYJ\n"
//     "KoZIhvcNAQEBBQADggEPADCCAQoCggEBAME0HKmIzfTOwkKLT3THHe+ObdizamPg\n"
//     "UZmD64Tf3zJdNeYGYn4CEXbyP6fy3tWc8S2boW6dzrH8SdFf9uo320GJA9B7U1FW\n"
//     "Te3xda/Lm3JFfaHjkWw7jBwcauQZjpGINHapHRlpiCZsquAthOgxW9SgDgYlGzEA\n"
//     "s06pkEFiMw+qDfLo/sxFKB6vQlFekMeCymjLCbNwPJyqyhFmPWwio/PDMruBTzPH\n"
//     "3cioBnrJWKXc3OjXdLGFJOfj7pP0j/dr2LH72eSvv3PQQFl90CZPFhrCUcRHSSxo\n"
//     "E6yjGOdnz7f6PveLIB574kQORwt8ePn0yidrTC1ictikED3nHYhMUOUCAwEAAaNT\n"
//     "MFEwHQYDVR0OBBYEFPVV6xBUFPiGKDyo5V3+Hbh4N9YSMB8GA1UdIwQYMBaAFPVV\n"
//     "6xBUFPiGKDyo5V3+Hbh4N9YSMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEL\n"
//     "BQADggEBAGa9kS21N70ThM6/Hj9D7mbVxKLBjVWe2TPsGfbl3rEDfZ+OKRZ2j6AC\n"
//     "6r7jb4TZO3dzF2p6dgbrlU71Y/4K0TdzIjRj3cQ3KSm41JvUQ0hZ/c04iGDg/xWf\n"
//     "+pp58nfPAYwuerruPNWmlStWAXf0UTqRtg4hQDWBuUFDJTuWuuBvEXudz74eh/wK\n"
//     "sMwfu1HFvjy5Z0iMDU8PUDepjVolOCue9ashlS4EB5IECdSR2TItnAIiIwimx839\n"
//     "LdUdRudafMu5T5Xma182OC0/u/xRlEm+tvKGGmfFcN0piqVl8OrSPBgIlb+1IKJE\n"
//     "m/XriWr/Cq4h/JfB7NTsezVslgkBaoU=\n"
//     "-----END CERTIFICATE-----";
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

int main() {

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
    result= network.connect(MQTT_HOST, MQTT_PORT, stack) ;
    if (result < 0) {
      printf("TCP with TLS: failed to connect to " MQTT_HOST " %d\n",
      result); return -1;
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

  while (true) {
    printf("Blink!\n");
    led = !led;
    ThisThread::sleep_for(BLINKING_RATE);
  }
}