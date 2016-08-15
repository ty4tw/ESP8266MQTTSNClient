/*============================================
 *
 *      MQTT-SN Client Application
 *
 *===========================================*/
#include "MqttsnClientApp.h"
#include "MqttsnClient.h"
#include "Payload.h"
#include "Timer.h"

using namespace std;
using namespace ESP8266MQTTSNClient;
/*------------------------------------------------------
 *          Network parameters
 *------------------------------------------------------*/
#include "NWconfig.h"

/*------------------------------------------------------
 *          MQTT-SN parameters
 *------------------------------------------------------*/
MQTTSN_CONFIG = {
    900,            //KeepAlive (seconds)
    true,           //Clean session
    0 ,             //Sleep duration in msecs (not available)
    "willTopic",    //WillTopic 
    "willMessage",  //WillMessage 
    0,              //WillQos
    false           //WillRetain
};

/*------------------------------------------------------
 *             Define Topic
 *------------------------------------------------------*/
const char* topic3 = "ty4tw/iot-2/evt/status/fmt/json";
const char* topic4 = "ty4tw/gpio01/on";

/*------------------------------------------------------
 *             Tasks invoked by Timer
 *------------------------------------------------------*/
static bool onoffFlg = true;

void task1(void)
{
 	Serial.print("TASK1 invoked\n");
 	char payload[100];
 	int payloadlen = sprintf((char*)payload,"1234567890abcdefghijklmn");
	PUBLISH(topic3,(uint8_t*)payload, payloadlen,1, false);
}

void task2(void)
{
  Serial.printf("UTC: %d\n",GETUTC());
}

/*---------------  List of task invoked by Timer ------------*/

TASK_LIST = {  //e.g. TASK( const char* topic, executing duration in seconds),
             TASK(task1,5),
             TASK(task2,10),
             END_OF_TASK_LIST
            };

/*------------------------------------------------------
 *       Tasks invoked by PUBLISH command Packet
 *------------------------------------------------------*/

int on_publish(uint8_t* pload, uint16_t ploadlen)
{
    Serial.print("ON_PUBLISH invoked.  ");
    pload[50] = 0;
    Serial.printf("payload:%s<---\n", pload);
    return 0;
}

/*------------ Link Callback to Topic -------------*/

SUBSCRIBE_LIST = {// e.g. SUB(topic, callback, QoS=0or1),
                  //SUB(topic3, on_publish, 0),
                  END_OF_SUBSCRIBE_LIST
                 };

/*------------------------------------------------------
 *            Tasks invoked by GPIO interruption
 *------------------------------------------------------*/
void interruptCallback(uint8_t gpioNo)
{
  Serial.printf("GPIO = %d \n", gpioNo);
  string id = theClient->getClientId();
  PUBLISH(topic4, (uint8_t*)id.c_str(), id.size(),1);
}

/*------------------------------------------------------
 *            initialize() function
 *------------------------------------------------------*/
 void setup(void)
 {
  Serial.begin(115200);
 }


