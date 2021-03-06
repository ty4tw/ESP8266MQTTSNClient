/*
 * MqttsnClient.h
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

#ifndef MQTTSNCLIENT_H_
#define MQTTSNCLIENT_H_

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "MqttsnClientApp.h"
#include "Timer.h"
#include "TaskManager.h"
#include "PublishManager.h"
#include "SubscribeManager.h"
#include "GwProxy.h"
#include "Payload.h"

using namespace std;

namespace ESP8266MQTTSNClient {

struct OnPublishList
{
	const char* topic;
	int (*pubCallback)(uint8_t* payload, uint16_t payloadlen);
	uint8_t qos;
};

#define GETNOW() Timer::getNow()
/*========================================
       Class MqttsnClient
 =======================================*/
class MqttsnClient{
public:
    MqttsnClient();
    ~MqttsnClient();
    void onConnect(void);
    void publish(const char* topicName, Payload* payload, uint8_t qos, bool retain = false);
    void publish(const char* topicName, uint8_t* payload, uint16_t len, uint8_t qos, bool retain = false);
    void publish(uint16_t topicId, Payload* payload, uint8_t qos, bool retain = false);
    void publish(uint16_t topicId, uint8_t* payload, uint16_t len, uint8_t qos, bool retain = false);
    void subscribe(const char* topicName, TopicCallback onPublish, uint8_t qos);
    void subscribe(uint16_t topicId, TopicCallback onPublish, uint8_t qos, uint8_t topicType);
    void unsubscribe(const char* topicName);
    void disconnect(uint16_t sleepInSecs);
    void initialize(NETCONF netconf, MqttsnConfig mqconf);
    void run(void);
    void networkClose(void);
    void registerInt0Callback(void (*callback)(uint8_t gpioNo));
    void checkGPIOInput(void);
    void addTask(void);
    void setSleepDuration(uint32_t duration);
    void sleep(void);
	const char* getClientId(void);
    GwProxy*          getGwProxy(void);
    PublishManager*   getPublishManager(void);
    SubscribeManager* getSubscribeManager(void);
    RegisterManager*  getRegisterManager(void);
    TaskManager*      getTaskManager(void);
    TopicTable*       getTopicTable(void);
    void              indicator(bool onoff);
private:
    uint8_t          readDigitalGPIO(uint8_t pinNo);
    TaskManager      _taskMgr;
    PublishManager   _pubMgr;
    SubscribeManager _subMgr;
    GwProxy          _gwProxy;
    uint32_t         _sleepDuration;
    uint8_t          _gpio0pre;
    void            (*_intCallback)(uint8_t gpioNo);
};

} /* ESP8266MQTTSNClient */
#endif /* MQTTSNCLIENT_H_ */
