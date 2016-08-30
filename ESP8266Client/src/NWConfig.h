/* MQTTSNClient's parameters */

/* Network config */
NETWORK_CONFIG  = {
    "ESP8266",           // ClientId prefix
    {225,1,1,1},         // Multicast group IP
    1883,                // Multicast group Port
    21000,               // Local PortNo
};

/* WiFi parameters */
AP_LIST = {
    {"SSID1", "PASSWD1"},
    {"SSID2", "PASSWD2"},
    END_OF_AP_LIST
};

/* SNTP parameters */
const char* theSNTPserver   = "ntp.nict.jp";
int         theSNTPinterval = 3600; //secs
int         theTimeDifference = +9;

/* OTA parameters */
int         theOTATimeout = 60;     //secs
const char* theOTAPasswd  = "1234";
int         theOTAportNo  = 8266;

extern MqttsnClient* theClient;
/* end of config */
