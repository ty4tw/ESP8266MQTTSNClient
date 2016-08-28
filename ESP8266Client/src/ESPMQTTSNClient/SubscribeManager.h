/*
 * SubscribeManager.h
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

#ifndef SUBSCRIBEMANAGER_H_
#define SUBSCRIBEMANAGER_H_

#include <stdio.h>
#include <string.h>

#include "MqttsnClientApp.h"
#include "RegisterManager.h"
#include "Timer.h"
#include "TopicTable.h"

using namespace std;

namespace ESP8266MQTTSNClient {

typedef struct SubElement{
    TopicCallback callback;
    const char* topicName;
    uint16_t  msgId;
    uint32_t  sendUTC;
    uint16_t  topicId;
    uint8_t   msgType;
    uint8_t   topicType;
    uint8_t   qos;

    int       retryCount;
    uint8_t   done;

    SubElement* prev;
    SubElement* next;
} SubElement;

/*========================================
       Class SubscribeManager
 =======================================*/
class SubscribeManager{
public:
    SubscribeManager();
    ~SubscribeManager();
    void onConnect(void);
    void subscribe(const char* topicName, TopicCallback onPublish, uint8_t qos);
    void subscribe(uint16_t topicId, TopicCallback onPublish, uint8_t qos, uint8_t topicType);
    void unsubscribe(const char* topicName);
    void unsubscribe(uint16_t topicId, uint8_t topicType);
    void responce(const uint8_t* msg);
    void checkTimeout(void);
    bool isDone(void);
private:
    void send(SubElement* elm);
    SubElement* getFirstElement(void);
    SubElement* getElement(uint16_t msgId);
    SubElement* getElement(uint16_t topicId, uint8_t topicType);
    SubElement* getElement(const char* topicName, uint8_t msgType);
	SubElement* add(uint8_t msgType, const char* topicName, uint16_t topicId, uint8_t topicType, uint8_t qos, TopicCallback callback);
	void remove(SubElement* elm);
	SubElement* _first;
	SubElement* _last;
};
 
} /* end of namespace */
#endif /* SUBSCRIBEMANAGER_H_ */
