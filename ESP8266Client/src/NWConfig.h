/* MQTTSNClient's parameters */

/* WiFi parameters */
const char* theSsid   = "SSID";
const char* thePasswd = "PASSWORD";

/* OTA parameters */
int         theOTATimeout = 120;   // seconds
const char* theOTAPasswd  = "1234";
uint16_t    theOTAportNo  = 8266;


/* Network config */
NETWORK_CONFIG  = {
    "ESP8266",           // ClientId prefix  < 15 chars
    {225,1,1,1},         // Multicast group IP
    1883,                // Multicast group Port
    2001,                // Local PortNo
};

/* SNTP parameters */
const char* theSNTPserver = "ntp.nict.jp";
int  theSNTPinterval = 3600;         // seconds



