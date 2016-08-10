/*============================================
 *
 *      MQTT-SN Client Application
 *
 *===========================================*/
#include <MqttsnClientApp.h>
#include <MqttsnClient.h>

using namespace std;
using namespace ESP8266MQTTSNClient;
extern MqttsnClient* theClient;
/*------------------------------------------------------
 *          Network parameters
 *------------------------------------------------------*/
#include "NWconfig.h"

/*------------------------------------------------------
 *             MQTT-SN parameters
 *------------------------------------------------------*/
MQTTSN_CONFIG = {
    900,            //KeepAlive (seconds)
    true,           //Clean session
    false,          //EndDevice
    "willTopic",    //WillTopic   or 0   DO NOT USE NULL STRING "" !
    "willMessage"   //WillMessage or 0   DO NOT USE NULL STRING "" !
};

/*------------------------------------------------------
 *             Define Topic
 *------------------------------------------------------*/
const char* topic1 = "ty4tw@github/onoff/arduino";
const char* topic2 = "ty4tw@github/onoff/linux";

/*------------------------------------------------------
 *             Tasks invoked by Timer
 *------------------------------------------------------*/
static bool onoffFlg = true;

void task1(void)
{
  Serial.print("TASK1 invoked\n");
  MQTTSNPayload* pl = new MQTTSNPayload(10);
  onoffFlg = !onoffFlg;
  pl->set_bool(onoffFlg);
  theClient->publish(topic1,pl,1);
  delete pl;
}

void task2(void)
{
  Serial.printf("UTC: %d\n",GETUTC());
}

/*---------------  List of task invoked by Timer ------------*/

TASK_LIST = {  //e.g. TASK( const char* topic, executing duration in second),
             TASK(task1,3),
             TASK(task2,10),
             END_OF_TASK_LIST
            };

/*------------------------------------------------------
 *       Tasks invoked by PUBLISH command Packet
 *------------------------------------------------------*/

int on_publish(uint8_t* pload, uint16_t ploadlen)
{
    Serial.print("ON_PUBLISH invoked.  ");
    MQTTSNPayload payload;
    payload.getPayload(pload, ploadlen)
    INDICATOR_ON(payload->get_bool(0));
    return 0;
}

/*------------ Link Callback to Topic -------------*/

SUBSCRIBE_LIST = {// e.g. SUB(topic, callback, QoS=0or1),
                  //SUB(topic1, on_publish, 1),
                  END_OF_SUBSCRIBE_LIST
                 };

/*------------------------------------------------------
 *            Tasks invoked by INT0 interruption
 *------------------------------------------------------*/
void interruptCallback(void)
{

}

/*------------------------------------------------------ 
 *            initialize() function
 *------------------------------------------------------*/
 void setup(void)
 {
  Serial.begin(115200);
 }


