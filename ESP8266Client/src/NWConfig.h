/* MQTTSNClient's parameters */

/* Network config */
NETWORK_CONFIG  = {
    "ESP8266",           // ClientId prefix
    {225,1,1,1},         // Multicast group IP
    1883,                // Multicast group Port
    21000,               // Local PortNo
};

/* WiFi parameters */
const char* theSsid   = "SSID";
const char* thePasswd = "PASSWORD";

/* SNTP parameters */
const char* theSNTPserver   = "ntp.nict.jp";
int         theSNTPinterval = 3600; //secs

/* OTA parameters */
int         theOTATimeout = 60;     //secs
const char* theOTAPasswd  = "1234";
uint16_t    theOTAportNo  = 8266;

extern MqttsnClient* theClient;
/* end of config */
