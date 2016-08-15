/*
 * PublishManager.cpp
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
#include "MqttsnClient.h"
#include "PublishManager.h"
#include "GwProxy.h"
#include "Timer.h"
#include "Payload.h"

using namespace std;
using namespace ESP8266MQTTSNClient;

extern void setUint16(uint8_t* pos, uint16_t val);
extern uint16_t getUint16(const uint8_t* pos);
extern void stackTraceEntry(const char* fileName, const char* funcName, const int line);
extern void stackTraceExit(const char* fileName, const char* funcName, const int line);
extern void stackTraceExitRc(const char* fileName, const char* funcName, const int line, void* rc);
extern MqttsnClient* theClient;
extern bool theOTAflag;
/*========================================
 Class PublishManager
 =======================================*/
const char* NULLCHAR = "";

PublishManager::PublishManager()
{
	_first = 0;
	_last = 0;
	_elmCnt = 0;
	_publishedFlg = SAVE_TASK_INDEX;
}

PublishManager::~PublishManager()
{
	PubElement* elm = _first;
	PubElement* sav = 0;
	while (elm)
	{
		sav = elm->next;
		if (elm != 0)
		{
			delElement(elm);
		}
		elm = sav;
	}
}

void PublishManager::publish(const char* topicName, Payload* payload, uint8_t qos, bool retain)
{
	publish(topicName, payload->getRowData(), payload->getLen(), qos, retain);
}


void PublishManager::publish(const char* topicName, uint8_t* payload, uint16_t len, uint8_t qos, bool retain)
{
	uint8_t topicType = MQTTSN_TOPIC_TYPE_NORMAL;
	if ( strlen(topicName) <= 2 )
	{
		topicType = MQTTSN_TOPIC_TYPE_SHORT;
	}
	publish(topicName, payload, len, qos, topicType, retain);
}

void PublishManager::publish(const char* topicName, uint8_t* payload, uint16_t len, uint8_t qos, uint8_t topicType, bool retain)
{
	FUNC_ENTRY;
	uint16_t msgId = 0;
	if ( qos > 0 )
	{
		msgId = theClient->getGwProxy()->getNextMsgId();
	}
	uint16_t topicId = theClient->getGwProxy()->getTopicTable()->getTopicId(topicName);
	PubElement* elm = add(topicName, topicId, payload, len, qos, retain, msgId, topicType);
	//D_MQTTLOG("sendPublish elm->status %d\n", elm->status);
	if (elm->status == TOPICID_IS_READY)
	{
		sendPublish(elm);
	}
	else
	{
		theClient->getGwProxy()->registerTopic((const char*) topicName, 0);
	}
exit:
	FUNC_EXIT;
	return;
}

void PublishManager::publish(uint16_t topicId, Payload* payload, uint8_t qos, bool retain)
{
	publish(topicId, payload->getRowData(), payload->getLen(), qos, retain);
}

void PublishManager::publish(uint16_t topicId, uint8_t* payload, uint16_t len, uint8_t qos, bool retain)
{
	FUNC_ENTRY;
	uint16_t msgId = 0;
	if ( qos > 0 )
	{
		msgId = theClient->getGwProxy()->getNextMsgId();
	}
	PubElement* elm = add(NULLCHAR, topicId, payload, len, qos, retain, msgId, MQTTSN_TOPIC_TYPE_PREDEFINED);
	sendPublish(elm);
exit:
	FUNC_EXIT;
	return;
}

void PublishManager::sendPublish(PubElement* elm)
{
	FUNC_ENTRY;

	uint8_t msg[MQTTSN_MAX_MSG_LENGTH + 1];
	uint8_t org = 0;

	if (elm == 0)
	{
		goto exit;
	}

	theClient->getGwProxy()->connect();

	if (elm->payloadlen > 128)
	{
		msg[0] = 0x01;
		setUint16(msg + 1, elm->payloadlen + 9);
		org = 2;
	}
	else
	{
		msg[0] = (uint8_t) elm->payloadlen + 7;
	}
	msg[org + 1] = MQTTSN_TYPE_PUBLISH;
	msg[org + 2] = elm->flag;
	if ((elm->retryCount < MQTTSN_RETRY_COUNT))
	{
		msg[org + 2] = msg[org + 2] | MQTTSN_FLAG_DUP;
	}
	if ((elm->flag & 0x03) == MQTTSN_TOPIC_TYPE_SHORT)
	{
		memcpy(msg + org + 3, elm->topicName, 2);
	}
	else
	{
		setUint16(msg + org + 3, elm->topicId);
	}
	setUint16(msg + org + 5, elm->msgId);
	memcpy(msg + org + 7, elm->payload, elm->payloadlen);

	if ( theClient->getGwProxy()->writeMsg(msg) == 0 )
	{
		goto exit;
	}
	theClient->getGwProxy()->resetPingReqTimer();

	D_MQTTLOG(" send %s msgId:%c%04x\n", "PUBLISH", msg[org + 2] & MQTTSN_FLAG_DUP ? '+' : ' ', elm->msgId);

	if ((elm->flag & 0x60) == MQTTSN_FLAG_QOS_0)
	{
		remove(elm);  // PUBLISH Done
		goto exit;
	}
	else if ((elm->flag & 0x60) == MQTTSN_FLAG_QOS_1)
	{
		elm->status = WAIT_PUBACK;
	}
	else if ((elm->flag & 0x60) == MQTTSN_FLAG_QOS_2)
	{
		elm->status = WAIT_PUBREC;
	}

	elm->sendUTC = time(NULL);
	elm->retryCount--;

exit:
	FUNC_EXIT;
	return;
}

void PublishManager::sendSuspend(const char* topicName, uint16_t topicId, uint8_t topicType)
{
	FUNC_ENTRY;
	PubElement* elm = _first;
	while (elm)
	{
		if (strcmp(elm->topicName, topicName) == 0 && elm->status == TOPICID_IS_SUSPEND)
		{
			elm->topicId = topicId;
			elm->flag |= topicType;
			elm->status = TOPICID_IS_READY;
			sendPublish(elm);
			elm = 0;
		}
		else
		{
			elm = elm->next;
		}
	}
exit:
	FUNC_EXIT;
	return;
}

void PublishManager::sendPubAck(uint16_t topicId, uint16_t msgId, uint8_t rc)
{
	FUNC_ENTRY;
	uint8_t msg[7];
	msg[0] = 7;
	msg[1] = MQTTSN_TYPE_PUBACK;
	setUint16(msg + 2, topicId);
	setUint16(msg + 4, msgId);
	msg[6] = rc;
	theClient->getGwProxy()->writeMsg(msg);
exit:
	FUNC_EXIT;
	return;
}

void PublishManager::sendPubRel(PubElement* elm)
{
	FUNC_ENTRY
	uint8_t msg[4];
	msg[0] = 4;
	msg[1] = MQTTSN_TYPE_PUBREL;
	setUint16(msg + 2, elm->msgId);
	theClient->getGwProxy()->writeMsg(msg);
exit:
	FUNC_EXIT;
	return;
}

bool PublishManager::isDone(void)
{
	return (_first == 0);
}

bool PublishManager::isMaxFlight(void)
{
	return (_elmCnt > MAX_INFLIGHT_MSG / 2);
}

void PublishManager::responce(const uint8_t* msg, uint16_t msglen)
{
	FUNC_ENTRY;
	if (msg[0] == MQTTSN_TYPE_PUBACK)
	{
		uint16_t msgId = getUint16(msg + 3);
		PubElement* elm = getElement(msgId);

		//D_MQTTLOG("PubElement elm %d\n", elm);
		if (elm == 0)
		{
			goto exit;;
		}
		if (msg[5] == MQTTSN_RC_ACCEPTED)
		{
			if (elm->status == WAIT_PUBACK)
			{
				remove(elm); // PUBLISH Done
			}
		}
		else if (msg[5] == MQTTSN_RC_REJECTED_INVALID_TOPIC_ID)
		{
			elm->status = TOPICID_IS_SUSPEND;
			elm->topicId = 0;
			elm->retryCount = MQTTSN_RETRY_COUNT;
			elm->sendUTC = 0;
			theClient->getGwProxy()->registerTopic((char*) elm->topicName, 0);
		}
	}
	else if (msg[0] == MQTTSN_TYPE_PUBREC)
	{
		PubElement* elm = getElement(getUint16(msg + 1));
		if (elm == 0)
		{
			goto exit;;
		}
		if (elm->status == WAIT_PUBREC || elm->status == WAIT_PUBCOMP)
		{
			sendPubRel(elm);
			elm->status = WAIT_PUBCOMP;
			elm->sendUTC = time(NULL);
		}
	}
	else if (msg[0] == MQTTSN_TYPE_PUBCOMP)
	{
		PubElement* elm = getElement(getUint16(msg + 1));
		if (elm == 0)
		{
			goto exit;;
		}
		if (elm->status == WAIT_PUBCOMP)
		{
			remove(elm);  // PUBLISH Done
		}
	}
exit:
	FUNC_EXIT;
	return;
}

void PublishManager::published(uint8_t* msg, uint16_t msglen)
{
	FUNC_ENTRY;
	if (msg[1] & MQTTSN_FLAG_QOS_1)
	{
		sendPubAck(getUint16(msg + 2), getUint16(msg + 4), MQTTSN_RC_ACCEPTED);
	}

	uint16_t topicId = getUint16(msg + 2);

	if ( (msg[1] & 0x03) == MQTTSN_TOPIC_TYPE_PREDEFINED )
	{
		if (topicId == MQTTSN_TOPICID_PREDEFINED_OTA)
		{
			theOTAflag = true;
		}
	}
	else
	{
		_publishedFlg = NEG_TASK_INDEX;
		theClient->getTopicTable()->execCallback(topicId, msg + 6, msglen - 6, msg[1] & 0x03);
		_publishedFlg = SAVE_TASK_INDEX;
	}
exit:
	FUNC_EXIT;
	return;
}

void PublishManager::checkTimeout(void)
{
	PubElement* elm = _first;
	while (elm)
	{
		if (elm->sendUTC > 0 && elm->sendUTC + MQTTSN_TIME_RETRY < time(NULL))
		{
			if (elm->retryCount >= 0)
			{
				sendPublish(elm);
				D_MQTTLOG("...Timeout retry\r\n");
			}
			else
			{
				theClient->getGwProxy()->reconnect();
				elm->retryCount = MQTTSN_RETRY_COUNT;
				break;
			}
		}
		elm = elm->next;
	}
}

PubElement* PublishManager::getElement(uint16_t msgId)
{
	FUNC_ENTRY;
	PubElement* elm = _first;
	//D_MQTTLOG("msgID: %d first %d  last %d", msgId, _first, _last);
	while (elm)
	{
		//D_MQTTLOG("mid: %d \n", elm->msgId);
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
	FUNC_EXIT(elm);
	return elm;
}

PubElement* PublishManager::getElement(const char* topicName)
{
	FUNC_ENTRY;
	PubElement* elm = _first;
	while (elm)
	{
		if (strcmp(elm->topicName, topicName) == 0)
		{
			goto exit;
		}
		else
		{
			elm = elm->next;
		}
	}
exit:
	FUNC_EXIT(elm);
	return elm;
}

void PublishManager::remove(PubElement* elm)
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
		delElement(elm);
	}

	//D_MQTTLOG("first %d  last %d\n", _first, _last);
exit:
	FUNC_EXIT;
}

void PublishManager::delElement(PubElement* elm)
{
	FUNC_ENTRY;
	if (elm->taskIndex >= 0)
	{
		theClient->getTaskManager()->done(elm->taskIndex);
	}
	_elmCnt--;
	if ( elm->payload )
	{
		free(elm->payload);
	}
	free(elm);
exit:
	FUNC_EXIT;
	return;
}


PubElement* PublishManager::add(const char* topicName, uint16_t topicId, uint8_t* payload, uint16_t len, uint8_t qos,
		uint8_t retain, uint16_t msgId, uint8_t topicType)
{
	FUNC_ENTRY;
	PubElement* rc = 0;
	PubElement* elm = (PubElement*) calloc(1, sizeof(PubElement));

	if (elm == 0)
	{
		rc = elm;
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

	elm->topicName = topicName;
	elm->flag |= topicType;

	if (qos == 0)
	{
		elm->flag |= MQTTSN_FLAG_QOS_0;
	}
	else if (qos == 1)
	{
		elm->flag |= MQTTSN_FLAG_QOS_1;
	}
	else if (qos == 2)
	{
		elm->flag |= MQTTSN_FLAG_QOS_2;
	}
	if (retain)
	{
		elm->flag |= MQTTSN_FLAG_RETAIN;
	}

	if (topicId)
	{
		elm->status = TOPICID_IS_READY;
		elm->topicId = topicId;
	}

	elm->payloadlen = len;
	elm->msgId = msgId;
	elm->retryCount = MQTTSN_RETRY_COUNT;
	elm->sendUTC = 0;

	if (_publishedFlg == NEG_TASK_INDEX)
	{
		elm->taskIndex = -1;
	}
	else
	{
		elm->taskIndex = theClient->getTaskManager()->getIndex();
		theClient->getTaskManager()->suspend(elm->taskIndex);
	}

	elm->payload = (uint8_t*) malloc(len);
	if (elm->payload == 0)
	{
		delElement(elm);
		rc = 0;
		goto exit;
	}
	memcpy(elm->payload, payload, len);

	++_elmCnt;
	rc = elm;

exit:
	FUNC_EXIT_RC(rc);
	return rc;
}
