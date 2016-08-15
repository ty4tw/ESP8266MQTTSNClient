/*
 * SubscribeManager.cpp
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

#include <stdlib.h>
#include <string.h>

#include "MqttsnClientApp.h"
#include "GwProxy.h"
#include "MqttsnClient.h"
#include "SubscribeManager.h"
#include "Timer.h"

using namespace std;
using namespace ESP8266MQTTSNClient;

extern void setUint16(uint8_t* pos, uint16_t val);
extern uint16_t getUint16(const uint8_t* pos);
extern void stackTraceEntry(const char* fileName, const char* funcName, const int line);
extern void stackTraceExit(const char* fileName, const char* funcName, const int line);
extern void stackTraceExitRc(const char* fileName, const char* funcName, const int line, void* rc);
extern MqttsnClient* theClient;
extern SUBSCRIBE_LIST;
#define SUB_DONE   1
#define SUB_READY  0
/*========================================
 Class SubscribeManager
 =======================================*/
SubscribeManager::SubscribeManager()
{
	_first = 0;
	_last = 0;
}

SubscribeManager::~SubscribeManager()
{
	FUNC_ENTRY;
	SubElement* elm = _first;
	SubElement* sav = 0;
	while (elm)
	{
		sav = elm->next;
		if (elm != 0)
		{
			free(elm);
		}
		elm = sav;
	}
exit:
	FUNC_EXIT;
}

void SubscribeManager::onConnect(void)
{
	FUNC_ENTRY;
	if (_first == 0)
	{
		for (uint8_t i = 0; theOnPublishList[i].topic != 0; i++)
		{
			subscribe(theOnPublishList[i].topic, theOnPublishList[i].pubCallback, theOnPublishList[i].qos);
		}
	}
	else
	{
		SubElement* elm = _first;
		SubElement* pelm;
		do
		{
			pelm = elm;
			if (elm->msgType == MQTTSN_TYPE_SUBSCRIBE)
			{
				elm->done = SUB_READY;
				elm->retryCount = MQTTSN_RETRY_COUNT;
				subscribe(elm->topicName, elm->callback, elm->qos);
			}
			elm = pelm->next;
		} while (pelm->next);
	}

	while (!theClient->getSubscribeManager()->isDone())
	{
		theClient->getGwProxy()->getMessage();
	}
exit:
	FUNC_EXIT;
	return;
}

bool SubscribeManager::isDone(void)
{
	//FUNC_ENTRY;
	SubElement* elm = _first;
	SubElement* prevelm;
	bool rc = true;

	//FUNC_ENTRY;
	while (elm)
	{
		prevelm = elm;
		if (elm->done == SUB_READY)
		{
			rc = false;
			goto exit;
		}
		elm = prevelm->next;
	}
exit:
	//FUNC_EXIT_RC(rc);
	return rc;
}

void SubscribeManager::send(SubElement* elm)
{
	FUNC_ENTRY;
	if (elm->done == SUB_DONE)
	{
		goto exit;
	}
	uint8_t msg[MQTTSN_MAX_MSG_LENGTH + 1];
	if (elm->topicType == MQTTSN_TOPIC_TYPE_NORMAL)
	{
		msg[0] = 5 + strlen(elm->topicName);
		strcpy((char*) msg + 5, elm->topicName);
	}
	else
	{
		msg[0] = 7;
		setUint16(msg + 5, elm->topicId);
	}
	msg[1] = elm->msgType;
	msg[2] = elm->qos | elm->topicType;
	if (elm->retryCount == MQTTSN_RETRY_COUNT)
	{
		elm->msgId = theClient->getGwProxy()->getNextMsgId();
	}

	if ((elm->retryCount < MQTTSN_RETRY_COUNT) && elm->msgType == MQTTSN_TYPE_SUBSCRIBE)
	{
		msg[2] = msg[2] | MQTTSN_FLAG_DUP;
	}

	setUint16(msg + 3, elm->msgId);

	theClient->getGwProxy()->connect();
	theClient->getGwProxy()->writeMsg(msg);
	theClient->getGwProxy()->resetPingReqTimer();
	elm->sendUTC = time(NULL);
	elm->retryCount--;
exit:
	FUNC_EXIT;
	return;
}

void SubscribeManager::subscribe(const char* topicName, TopicCallback onPublish, uint8_t qos)
{
	FUNC_ENTRY;
	uint8_t topicType = MQTTSN_TOPIC_TYPE_NORMAL;
	if ( strlen(topicName) <= 2)
	{
		topicType = MQTTSN_TOPIC_TYPE_SHORT;
	}
	SubElement* elm = add(MQTTSN_TYPE_SUBSCRIBE, topicName, 0, topicType, qos, onPublish);
	send(elm);
exit:
	FUNC_EXIT;
	return;
}

void SubscribeManager::subscribe(uint16_t topicId, TopicCallback onPublish, uint8_t qos, uint8_t topicType)
{
	FUNC_ENTRY;
	SubElement* elm = add(MQTTSN_TYPE_SUBSCRIBE, 0, topicId, topicType, qos, onPublish);
	send(elm);
exit:
	FUNC_EXIT;
	return;
}

void SubscribeManager::unsubscribe(const char* topicName)
{
	FUNC_ENTRY;
	SubElement* elm = add(MQTTSN_TYPE_UNSUBSCRIBE, topicName, 0, MQTTSN_TOPIC_TYPE_NORMAL, 0, 0);
	send(elm);
exit:
	FUNC_EXIT;
	return;
}

void SubscribeManager::unsubscribe(uint16_t topicId, uint8_t topicType)
{
	FUNC_ENTRY;
	SubElement* elm = add(MQTTSN_TYPE_UNSUBSCRIBE, 0, topicId, topicType, 0, 0);
	send(elm);
exit:
	FUNC_EXIT;
	return;
}

void SubscribeManager::checkTimeout(void)
{
	//FUNC_ENTRY;
	SubElement* elm = _first;

	while (elm)
	{
		if (elm->sendUTC + MQTTSN_TIME_RETRY < time(NULL))
		{
			if (elm->retryCount >= 0)
			{
				send(elm);
			}
			else
			{
				if ( elm->done == SUB_READY )
				{
					elm->done = SUB_DONE;
				}
			}
		}
		elm = elm->next;
	}
exit:
	//FUNC_EXIT;
	return;
}

void SubscribeManager::responce(const uint8_t* msg)
{
	FUNC_ENTRY;
	if (msg[0] == MQTTSN_TYPE_SUBACK)
	{
		uint16_t topicId = getUint16(msg + 2);
		uint16_t msgId = getUint16(msg + 4);
		uint8_t rc = msg[6];

		TopicTable* tt = theClient->getGwProxy()->getTopicTable();
		SubElement* elm = getElement(msgId);
		if (elm)
		{
			if ( rc == MQTTSN_RC_ACCEPTED )
			{
				tt->add((const char*) elm->topicName, topicId, elm->topicType, elm->callback);
				getElement(msgId)->done = SUB_DONE;
			}
			else
			{
				remove(elm);
			}
		}
	}
	else if (msg[0] == MQTTSN_TYPE_UNSUBACK)
	{
		uint16_t msgId = getUint16(msg + 1);
		SubElement* elm = getElement(msgId);
		if (elm)
		{
			TopicTable* tt = theClient->getGwProxy()->getTopicTable();
			tt->setCallback((const char*)elm->topicName, 0);
			remove(getElement(msgId));
		}
		else
		{
			remove(getElement(msgId));
		}
	}
exit:
	FUNC_EXIT;
	return;
}

/* SubElement operations */

SubElement* SubscribeManager::add(uint8_t msgType, const char* topicName, uint16_t topicId, uint8_t topicType,
		uint8_t qos, TopicCallback callback)
{
	FUNC_ENTRY;
	SubElement* elm = getElement(topicName, msgType);

	if ( elm  == 0 )
	{
		elm = (SubElement*)calloc(1, sizeof(SubElement));
		if (elm == 0)
		{
			goto exit;
		}
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
	}
	elm->msgType = msgType;
	elm->callback = callback;
	elm->topicName = topicName;
	elm->topicId = topicId;
	elm->topicType = topicType;
	if (qos == 1)
	{
		elm->qos = MQTTSN_FLAG_QOS_1;
	}
	else if (qos == 2)
	{
		elm->qos = MQTTSN_FLAG_QOS_2;
	}
	else
	{
		elm->qos = MQTTSN_FLAG_QOS_0;
	}
	elm->msgId = 0;
	elm->retryCount = MQTTSN_RETRY_COUNT;
	elm->done = SUB_READY;
	elm->sendUTC = 0;
exit:
	FUNC_EXIT_RC(elm);
	return elm;
}

void SubscribeManager::remove(SubElement* elm)
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

SubElement* SubscribeManager::getElement(uint16_t msgId)
{
	FUNC_ENTRY;
	SubElement* elm = _first;
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
exit:
	FUNC_EXIT_RC(elm);
	return elm;
}

SubElement* SubscribeManager::getElement(const char* topicName, uint8_t msgType)
{
	SubElement* elm = _first;
	while (elm)
	{
		if (strcmp(elm->topicName, topicName) == 0 && elm->msgType == msgType)
		{
			goto exit;
		}
		else
		{
			elm = elm->next;
		}
	}
exit:
	FUNC_EXIT_RC(elm);
	return elm;
}

SubElement* SubscribeManager::getElement(uint16_t topicId, uint8_t topicType)
{
	FUNC_ENTRY;
	SubElement* elm = _first;
	while (elm)
	{
		if (elm->topicId == topicId && elm->topicType == topicType)
		{
			goto exit;
		}
		else
		{
			elm = elm->next;
		}
	}
exit:
	FUNC_EXIT_RC(elm);
	return elm;
}
