/*
 * RegisterManager.cpp
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
#include <RegisterManager.h>
#include <MqttsnClient.h>
#include <GwProxy.h>
#include <Timer.h>

#include <stdlib.h>
#include <string.h>

using namespace std;
using namespace ESP8266MQTTSNClient;

extern void setUint16(uint8_t* pos, uint16_t val);
extern void stackTraceEntry(const char* fileName, const char* funcName, const int line);
extern void stackTraceExit(const char* fileName, const char* funcName, const int line);
extern void stackTraceExitRc(const char* fileName, const char* funcName, const int line, void* rc);
extern MqttsnClient* theClient;
/*=====================================
 Class RegisterQue
 =====================================*/
RegisterManager::RegisterManager()
{
	_first = 0;
	_last = 0;
}

RegisterManager::~RegisterManager()
{
	RegQueElement* elm = _first;
	RegQueElement* sav = 0;
	while (elm)
	{
		sav = elm->next;
		if (elm != 0)
		{
			free(elm);
		}
		elm = sav;
	}
}

bool RegisterManager::isDone(void)
{
	return _first == 0;
}

const char* RegisterManager::getTopic(uint16_t msgId)
{
	FUNC_ENTRY;
	const char* rc = 0;
	RegQueElement* elm = _first;
	while (elm)
	{
		if (elm->msgId == msgId)
		{
			rc = elm->topicName;
			goto exit;;
		}
		else
		{
			elm = elm->next;
		}
	}
	exit: FUNC_EXIT_RC(rc);
	return rc;
}

void RegisterManager::registerTopic(const char* topicName)
{
	FUNC_ENTRY;
	RegQueElement* elm = getElement(topicName);
	if (elm == 0)
	{
		uint16_t msgId = theClient->getGwProxy()->getNextMsgId();
		elm = add(topicName, msgId);
		send(elm);
	} FUNC_EXIT;
}

void RegisterManager::responceRegAck(uint16_t msgId, uint16_t topicId)
{
	FUNC_ENTRY;
	const char* topicName = getTopic(msgId);
	if (topicName)
	{
		uint8_t topicType = strlen((const char*) topicName) > 2 ? MQTTSN_TOPIC_TYPE_NORMAL : MQTTSN_TOPIC_TYPE_SHORT;
		theClient->getGwProxy()->getTopicTable()->setTopicId((const char*) topicName, topicId, topicType); // Add Topic to TopicTable
		RegQueElement* elm = getElement(msgId);
		remove(elm);
		D_MQTTLOG("\n");  // gwProxy. getMessage() needs this line.
		theClient->getPublishManager()->sendSuspend((const char*) topicName, topicId, topicType);
	}
	FUNC_EXIT;
}

void RegisterManager::responceRegister(uint8_t* msg, uint16_t msglen)
{
	FUNC_ENTRY;
	// *msg is terminated with 0x00 by Network::getMessage()
	uint8_t regack[7];
	regack[0] = 7;
	regack[1] = MQTTSN_TYPE_REGACK;
	memcpy(regack + 2, msg + 2, 4);

	Topic* tp = theClient->getGwProxy()->getTopicTable()->match((const char*) msg + 5);
	if (tp)
	{
		TopicCallback callback = tp->getCallback();
		void* topicName = calloc(strlen((char*) msg + 5) + 1, sizeof(char));
		theClient->getGwProxy()->getTopicTable()->add((const char*) topicName, 0, MQTTSN_TOPIC_TYPE_NORMAL, callback,
				1);
		regack[6] = MQTTSN_RC_ACCEPTED;
	}
	else
	{
		regack[6] = MQTTSN_RC_REJECTED_INVALID_TOPIC_ID;
	}
	theClient->getGwProxy()->writeMsg(regack);
	FUNC_EXIT;
}

uint8_t RegisterManager::checkTimeout(void)
{
	RegQueElement* sav = 0;
	RegQueElement* elm = _first;
	while (elm)
	{
		if (elm->sendUTC + MQTTSN_TIME_RETRY < time(NULL))
		{
			if (elm->retryCount >= 0)
			{
				send(elm);
				D_MQTTLOG("...Timeout retry\r\n");
			}
			else
			{
				if (elm->next)
				{
					sav = elm->prev;
					remove(elm);
					if (sav)
					{
						elm = sav;
					}
					else
					{
						break;
					}
				}
				else
				{
					remove(elm);
					break;
				}
			}
		}
		elm = elm->next;
	}
	return 0;
}

void RegisterManager::send(RegQueElement* elm)
{
	FUNC_ENTRY;
	uint8_t msg[MQTTSN_MAX_MSG_LENGTH + 1];
	msg[0] = 6 + strlen(elm->topicName);
	msg[1] = MQTTSN_TYPE_REGISTER;
	msg[2] = msg[3] = 0;
	setUint16(msg + 4, elm->msgId);
	strcpy((char*) msg + 6, elm->topicName);
	theClient->getGwProxy()->connect();
	theClient->getGwProxy()->writeMsg(msg);
	elm->sendUTC = time(NULL);
	elm->retryCount--;
	FUNC_EXIT;
}

RegQueElement* RegisterManager::add(const char* topic, uint16_t msgId)
{
	FUNC_ENTRY;
	RegQueElement* elm = (RegQueElement*) calloc(1, sizeof(RegQueElement));

	if (elm)
	{
		if (_last == 0)
		{
			_first = elm;
			_last = elm;
		}
		else
		{
			elm->prev = _last;
			_last->next = elm;
			_last = elm;
		}
		elm->topicName = topic;
		elm->msgId = msgId;
		elm->retryCount = MQTTSN_RETRY_COUNT;
		elm->sendUTC = 0;
	}
	exit: FUNC_EXIT_RC(elm);
	return elm;
}

void RegisterManager::remove(RegQueElement* elm)
{
	FUNC_ENTRY;
	if (elm)
	{
		if (elm->prev == 0)
		{
			_first = elm->next;
			if (elm->next == 0)
			{
				_last = 0;
			}
			else
			{
				elm->next->prev = 0;
				_last = elm->next;
			}
		}
		else
		{
			if ( elm->next == 0 )
			{
				_last = elm->prev;
			}
			elm->prev->next = elm->next;
		}
		free(elm);
	}
exit:
	FUNC_EXIT;
	return;
}

RegQueElement* RegisterManager::getElement(const char* topicName)
{
FUNC_ENTRY;
RegQueElement* elm = _first;
while (elm)
{
	if (strcmp(elm->topicName, topicName))
	{
		elm = elm->next;
	}
	else
	{
		return elm;
	}
}
exit: FUNC_EXIT_RC(elm)
return elm;
}

RegQueElement* RegisterManager::getElement(uint16_t msgId)
{
FUNC_ENTRY;
RegQueElement* elm = _first;
while (elm)
{
	if (elm->msgId == msgId)
	{
		goto exit;
	}
	else
	{
		elm = elm->next;
	}
}
exit: FUNC_EXIT_RC(elm);
return elm;
}

