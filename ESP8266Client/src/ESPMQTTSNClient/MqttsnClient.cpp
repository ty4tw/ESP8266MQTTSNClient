/*
 * MqttsnClient.cpp
 *                      The BSD License
 *
 *           Copyright (c) 2015, tomoaki@tomy-tech.com
 *                    All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <MqttsnClientApp.h>
#include <MqttsnClient.h>
#include <GwProxy.h>
#include <Timer.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
//#include <WiFiUdp.h>

#if defined(ARDUINO) && ARDUINO >= 100
        #include <Arduino.h>
#endif

#if defined(ARDUINO) && ARDUINO < 100
        #include <WProgram.h>
#endif

using namespace std;
using namespace ESP8266MQTTSNClient;

extern void interruptCallback(void);
extern NETWORK_CONFIG;
extern MQTTSN_CONFIG;
extern TaskList      theTaskList[];
extern OnPublishList theOnPublishList[];
extern const char* theTopicOTA;
/*=====================================
          MqttsnClient
 ======================================*/
MqttsnClient* theClient = new MqttsnClient();
bool theOTA = false;
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

void loop(){
    theClient->registerInt0Callback(interruptCallback);
	theClient->addTask();
	theClient->initialize(theNetworkConfig, theMqttsnConfig);
	theClient->setSleepMode(theMqttsnConfig.sleep);

	while(true){
		if (!theOTA) {
			theClient->run();
		}else{
			WiFi.mode(WIFI_AP_STA);
			MDNS.begin(theClient->getClientId());
  			httpUpdater.setup(&httpServer);
  			httpServer.begin();
  			MDNS.addService("http", "tcp", 80);
  			Serial.printf("Update ready! Open http://%s.local/update in your browser\n", theClient->getClientId());	

			theOTA = false;
			Timer  timeout;
			timeout.start(600000);
			for (;;) 
			{
				httpServer.handleClient();
				if ( timeout.isTimeUp() )
				{
					ESP.reset();
				}
			}  
		}	
	}
}

int setOTAmode(MQTTSNPayload* pload)
{
  theOTA = true;
  return 0;
}

/*=====================================
        Class MqttsnClient
 ======================================*/
MqttsnClient::MqttsnClient(){
    _intCallback = 0;
}

MqttsnClient::~MqttsnClient(){
    
}

void MqttsnClient::initialize(NETCONF netconf, MqttsnConfig mqconf){
	pinMode(ARDUINO_LED_PIN,OUTPUT);
	_gwProxy.initialize(netconf, mqconf);
}

void MqttsnClient::registerInt0Callback(void (*callback)()){
    _intCallback = callback;
}

void MqttsnClient::addTask(void){
    _taskMgr.add(theTaskList);
}

void MqttsnClient::setSleepMode(bool mode){
    _sleepMode = mode;
}

GwProxy* MqttsnClient::getGwProxy(void){
    return &_gwProxy;
}

PublishManager* MqttsnClient::getPublishManager(void){
    return &_pubMgr;
};

SubscribeManager* MqttsnClient::getSubscribeManager(void){
    return &_subMgr;
};

RegisterManager*  MqttsnClient::getRegisterManager(void){
	return _gwProxy.getRegisterManager();
}

TaskManager* MqttsnClient::getTaskManager(void){
    return &_taskMgr;
};

TopicTable* MqttsnClient::getTopicTable(void){
    return _gwProxy.getTopicTable();
}

void MqttsnClient::publish(const char* topicName, MQTTSNPayload* payload, uint8_t qos, bool retain){
    _pubMgr.publish(topicName, payload, qos, retain);
}

void MqttsnClient::subscribe(const char* topicName, TopicCallback onPublish, uint8_t qos){
    _subMgr.subscribe(topicName, onPublish, qos);
}

void MqttsnClient::subscribe(uint16_t topicId, TopicCallback onPublish, uint8_t qos, uint8_t topicType){
    _subMgr.subscribe(topicId, onPublish, qos, topicType);
}

void  MqttsnClient::unsubscribe(const char* topicName){
    _subMgr.unsubscribe(topicName);
}

void MqttsnClient::disconnect(uint16_t sleepInSecs){
    _gwProxy.disconnect(sleepInSecs);
}

void MqttsnClient::run(void){
    _taskMgr.run();
    if (sleep()){
        _intCallback();
    }
}

void MqttsnClient::onConnect(void){

	/*
	 *    subscribe() for Predefined TopicId
	 */
	//subscribe(MQTTSN_TOPICID_PREDEFINED_TIME, setUTC, 0, MQTTSN_TOPIC_TYPE_PREDEFINED);

	for(uint8_t i = 0; theOnPublishList[i].pubCallback; i++){
		subscribe(theOnPublishList[i].topic, theOnPublishList[i].pubCallback, theOnPublishList[i].qos);
	}
	subscribe(theTopicOTA, setOTAmode, 1);
}

char* MqttsnClient::getClientId(void)
{
	return _gwProxy.getClientId();
}

void MqttsnClient::indicator(bool onOff){
#ifdef ARDUINO
	if (onOff){
		digitalWrite(ARDUINO_LED_PIN, 1);
	}else{
		digitalWrite(ARDUINO_LED_PIN, 0);
	}
#endif
}


#ifdef ARDUINO

//https://github.com/LowPowerLab/LowPower



int MqttsnClient::sleep(void){
	// Enter idle state for 8 s with the rest of peripherals turned off
	// Each microcontroller comes with different number of peripherals
	// Comment off line of code where necessary

#define SLEEP_TIME SLEEP_1S
	uint32_t sec = 1;
/*
	// ATmega328P, ATmega168
	//LowPower.idle(SLEEP_TIME, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF,
				SPI_OFF, USART0_OFF, TWI_OFF);
	//Timer::setUnixTime(Timer::getUnixTime() + sec);

	// ATmega32U4
	//LowPower.idle(SLEEP_TIME, ADC_OFF, TIMER4_OFF, TIMER3_OFF, TIMER1_OFF,
	//		  TIMER0_OFF, SPI_OFF, USART1_OFF, TWI_OFF, USB_OFF);
	//Timer::setUnixTime(Timer::getUnixTime() + sec);

	// ATmega2560
	//LowPower.idle(SLEEP_TIME, ADC_OFF, TIMER5_OFF, TIMER4_OFF, TIMER3_OFF,
	//		  TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART3_OFF,
	//		  USART2_OFF, USART1_OFF, USART0_OFF, TWI_OFF);
	//Timer::setUnixTime(Timer::getUnixTime() + sec);
*/
	return 0;
}

#else

int MqttsnClient::sleep(void){
    return 0;
}

#endif




