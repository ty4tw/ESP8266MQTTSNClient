#include <MqttsnClientApp.h>
#include <MqttsnClient.h>

using namespace std;
using namespace ESP8266MQTTSNClient;

extern MqttsnClient* theClient;

/*============================================
 *
 *      MQTT-SN Client Application
 *
 *===========================================*/

UDP_APP_CONFIG = 
{
    {
        "ESP8266-01",       //ClientId
        {225,1,1,1},        // Multicast group IP
        1883,               // Multicast group Port
        12001,              // Local PortNo
    },
    {
        300,            //KeepAlive
        true,           //Clean session
        false,          //EndDevice
        "willTopic",    //WillTopic   or 0   DO NOT USE NULL STRING "" !
        "willMessage"   //WillMessage or 0   DO NOT USE NULL STRING "" !
    }
 };

const char* theSsid = "--SSID--";
const char* thePasswd = "--PASS WORD--";

/*------------------------------------------------------
 *             Create Topic
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
  PUBLISH(topic1,pl,1);
}

void task2(void)
{

}

/*---------------  List of task invoked by Timer ------------*/

TASK_LIST = {  //e.g. TASK( const char* topic, executing duration in second),
             TASK(task1,3),
             TASK(task2,20),
             END_OF_TASK_LIST
            };

/*------------------------------------------------------
 *       Tasks invoked by PUBLISH command Packet
 *------------------------------------------------------*/

int on_publish(MQTTSNPayload* pload)
{
    Serial.print("ON_PUBLISH invoked.  ");
    INDICATOR_ON(pload->get_bool(0));
    return 0;
}

/*------------ Link Callback to Topic -------------*/

SUBSCRIBE_LIST = {// e.g. SUB(topic, callback, QoS=0or1),
                  SUB(topic1, on_publish, 1),
                  END_OF_SUBSCRIBE_LIST
                 };

/*------------------------------------------------------
 *            Tasks invoked by INT0 interruption
 *------------------------------------------------------*/
void interruptCallback(void){

}

/*------------------------------------------------------
 *            initialize() function
 *------------------------------------------------------*/
 void initialize(void)
 {
  Serial.begin(115200);
 }


