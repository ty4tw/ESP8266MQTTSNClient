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

#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoOTA.h>
#include <Arduino.h>

#include "MqttsnClientApp.h"
#include "MqttsnClient.h"
#include "GwProxy.h"
#include "Timer.h"
#include "Payload.h"

using namespace std;
using namespace ESP8266MQTTSNClient;

extern NETWORK_CONFIG;
extern MQTTSN_CONFIG;
extern TaskList theTaskList[];
extern OnPublishList theOnPublishList[];
extern const char* theOTAPasswd;
extern uint16_t theOTAportNo;
extern const char* theSsid;
extern const char* thePasswd;
extern const char* theSNTPserver;
extern int theSNTPinterval;
extern int theOTATimeout;
extern int theTimeDifference;
extern void interruptCallback(uint8_t gpioNo);
extern "C" {
#include <user_interface.h>
#include <gpio.h>
}
/*=====================================
       MqttsnClient
 ======================================*/
MqttsnClient* theClient = new MqttsnClient();

/*=====================================
 Class MqttsnClient
 ======================================*/
MqttsnClient::MqttsnClient()
{
	_intCallback = 0;
	_gpio0pre = 1;
}

MqttsnClient::~MqttsnClient()
{

}

void MqttsnClient::initialize(NETCONF netconf, MqttsnConfig mqconf)
{
	pinMode(ARDUINO_LED_PIN, OUTPUT);
	_gwProxy.initialize(netconf, mqconf);
}

void MqttsnClient::networkClose(void)
{
	_gwProxy.networkClose();
}

void MqttsnClient::registerInt0Callback(void (*callback)(uint8_t))
{
	_intCallback = callback;
}

void MqttsnClient::checkGPIOInput(void)
{
	uint8_t val, chkval;
	do
	{
		val = digitalRead(0);
		delay(20);
		chkval = digitalRead(0);
		delay(20);
	}
	while ( val != chkval );

	if ( val == 1 && _gpio0pre == 0 )
	{
		_intCallback(0);
		_gpio0pre = 1;
	}
	else if ( val == 0 )
	{
		_gpio0pre = 0;
	}

}


void MqttsnClient::addTask(void)
{
	_taskMgr.add(theTaskList);
}

GwProxy* MqttsnClient::getGwProxy(void)
{
	return &_gwProxy;
}

PublishManager* MqttsnClient::getPublishManager(void)
{
	return &_pubMgr;
}


SubscribeManager* MqttsnClient::getSubscribeManager(void)
{
	return &_subMgr;
}
;

RegisterManager* MqttsnClient::getRegisterManager(void)
{
	return _gwProxy.getRegisterManager();
}

TaskManager* MqttsnClient::getTaskManager(void)
{
	return &_taskMgr;
}
;

TopicTable* MqttsnClient::getTopicTable(void)
{
	return _gwProxy.getTopicTable();
}

void MqttsnClient::publish(const char* topicName, Payload* payload, uint8_t qos, bool retain)
{
	_pubMgr.publish(topicName, payload, qos, retain);
}

void MqttsnClient::publish(const char* topicName, uint8_t* payload, uint16_t len, uint8_t qos, bool retain)
{
	_pubMgr.publish(topicName, payload, len, qos, retain);
}

void MqttsnClient::publish(uint16_t topicId, Payload* payload, uint8_t qos, bool retain)
{
	_pubMgr.publish(topicId, payload, qos, retain);
}

void MqttsnClient::publish(uint16_t topicId, uint8_t* payload, uint16_t len, uint8_t qos, bool retain)
{
	_pubMgr.publish(topicId, payload, len, qos, retain);
}



void MqttsnClient::subscribe(const char* topicName, TopicCallback onPublish, uint8_t qos)
{
	_subMgr.subscribe(topicName, onPublish, qos);
}

void MqttsnClient::subscribe(uint16_t topicId, TopicCallback onPublish, uint8_t qos, uint8_t topicType)
{
	_subMgr.subscribe(topicId, onPublish, qos, topicType);
}

void MqttsnClient::unsubscribe(const char* topicName)
{
	_subMgr.unsubscribe(topicName);
}

void MqttsnClient::disconnect(uint16_t sleepInSecs)
{
	_gwProxy.disconnect(sleepInSecs);
}

void MqttsnClient::run(void)
{
	_taskMgr.run();
	sleep();
}

void MqttsnClient::onConnect(void)
{
	/*   subscribe topics in SUBSCRIBE_LIST  */
	for (uint8_t i = 0; theOnPublishList[i].pubCallback; i++)
	{
		subscribe(theOnPublishList[i].topic, theOnPublishList[i].pubCallback, theOnPublishList[i].qos);
	}
}

const char* MqttsnClient::getClientId(void)
{
	return _gwProxy.getClientId();
}

void MqttsnClient::indicator(bool onOff)
{
	if (onOff)
	{
		digitalWrite(ARDUINO_LED_PIN, 1);
	}
	else
	{
		digitalWrite(ARDUINO_LED_PIN, 0);
	}
}

void MqttsnClient::setSleepDuration(uint32_t duration)
{
	_sleepDuration = duration;
}

void sleep_wakeup(void)
{
		wifi_fpm_close();
		wifi_set_opmode(STATION_MODE);
		wifi_station_connect();
}

void MqttsnClient::sleep(void)
{
	if (_sleepDuration)
	{
		wifi_station_disconnect();
		wifi_set_opmode(NULL_MODE);
		wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
		wifi_fpm_open();
		wifi_fpm_set_wakeup_cb(sleep_wakeup);
		wifi_fpm_do_sleep(_sleepDuration * 1000);
	}
}


/*=====================================
            OTA Setup
 ======================================*/
bool theOTAflag = false;
bool theInitialize_done = false;

void setOTA(void)
{
	ArduinoOTA.setHostname(theClient->getClientId());
	ArduinoOTA.setPassword(theOTAPasswd);
	ArduinoOTA.setPort(theOTAportNo);

	ArduinoOTA.onStart([]()
	{
		D_OTALOG("Start");
		return;
	});
	ArduinoOTA.onEnd([]()
	{
		D_OTALOG("\nEnd");
		return;
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
	{
		D_OTALOG("Progress: %u%%\r", (progress / (total / 100)));
		return;
	});
	ArduinoOTA.onError([](ota_error_t error)
	{
		D_OTALOG("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) D_OTALOG("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) D_OTALOG("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) D_OTALOG("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) D_OTALOG("Receive Failed");
		else if (error == OTA_END_ERROR) D_OTALOG("End Failed");
		return;
	});
	ArduinoOTA.begin();
	D_OTALOG("\nOTA Ready!  IP address: ");
	D_OTALOG("%s\n", WiFi.localIP().toString().c_str());
}

/*=====================================
             loop()
 ======================================*/
void loop()
{
	if ( !theInitialize_done )
	{
		gpio_init();
		theClient->addTask();
		theClient->registerInt0Callback(interruptCallback);
		theClient->initialize(theNetworkConfig, theMqttsnConfig);
		Timer::initialize(theTimeDifference, 0, theSNTPserver, NULL, NULL, theSNTPinterval);
		theClient->setSleepDuration(theMqttsnConfig.sleepDuration);
		theInitialize_done = true;
	}

	Timer::update();
	theClient->run();
	if (theOTAflag)
	{
		Payload* pl = new Payload(128);
		if ( pl )
		{
			D_OTALOG("OTA start prepair\n");
			pl->set_str(theClient->getClientId());
			pl->set_str(WiFi.localIP().toString().c_str());
			pl->set_uint32(theOTAportNo);
			theClient->publish(MQTTSN_TOPICID_PREDEFINED_OTA_RESP, pl, 1,
					           MQTTSN_TOPIC_TYPE_PREDEFINED);
			theClient->run();
			theClient->networkClose();
			setOTA();
			for (int i = 0; i < theOTATimeout * 100; i++)
			{
				ArduinoOTA.handle();
				delay(10);
			}
			D_OTALOG("Timeout!!!");
			ESP.reset();
		}
		theOTAflag = false;
	}
}



