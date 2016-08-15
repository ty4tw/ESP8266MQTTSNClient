/*
 * TopicTable.h
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
#include <stdlib.h>

#include "MqttsnClientApp.h"
#include "TopicTable.h"

using namespace std;
using namespace ESP8266MQTTSNClient;
/*=====================================
        Class Topic
 ======================================*/
Topic::Topic(){
    _topicStr = 0;
    _callback = 0;
    _topicId = 0;
    _topicType = MQTTSN_TOPIC_TYPE_NORMAL;
    _next = 0;
    _malocFlg = 0;
}

Topic::~Topic(){
	if (_malocFlg){
		free(_topicStr);
	}
}

TopicCallback Topic::getCallback(void){
	return _callback;
}

int Topic::execCallback(uint8_t* payload, uint16_t payloadlen){
    if(_callback != 0){
        return _callback(payload, payloadlen);
    }
    return 0;
}


uint8_t Topic::hasWildCard(uint8_t* pos){
	*pos = strlen(_topicStr) - 1;
    if (*(_topicStr + *pos) == '#'){
        return MQTTSN_TOPIC_MULTI_WILDCARD;
    }else{
    	for(uint8_t p = 0; p < strlen(_topicStr); p++){
    		if (*(_topicStr + p) == '+'){
    			*pos = p;
    			return MQTTSN_TOPIC_SINGLE_WILDCARD;
    		}
    	}
    }
    return 0;
}

bool Topic::isMatch(const char* topic){
    uint8_t pos;

	if ( strlen(topic) < strlen(_topicStr)){
		return false;
	}

	uint8_t wc = hasWildCard(&pos);

	if (wc == MQTTSN_TOPIC_SINGLE_WILDCARD){
		if ( strncmp(_topicStr, topic, pos - 1) == 0){
			if (*(_topicStr + pos + 1) == '/'){
				for(uint8_t p = pos; p < strlen(topic); p++){
					if (*(topic + p) == '/'){
						if (strcmp(_topicStr + pos + 1, topic + p ) == 0){
							return true;
						}
					}
				}
			}else{
				for(uint8_t p = pos + 1;p < strlen(topic); p++){
					if (*(topic + p) == '/'){
						return false;
					}
				}
			}
			return true;
		}
	}else if (wc == MQTTSN_TOPIC_MULTI_WILDCARD){
		if (strncmp(_topicStr, topic, pos) == 0){
			return true;
		}
	}else if (strcmp(_topicStr, topic) == 0){
		return true;
	}
	return false;
}


/*=====================================
        Class TopicTable
 ======================================*/
TopicTable::TopicTable(){
	_first = 0;
	_last = 0;
}

TopicTable::~TopicTable(){
	clearTopic();
}


Topic* TopicTable::getTopic(const char* topic){
	Topic* p = _first;
	while(p){
		if (p->_topicStr != 0 && strcmp(p->_topicStr, topic) == 0){
			return p;
		}
		p = p->_next;
	}
	return 0;
}

Topic* TopicTable::getTopic(uint16_t topicId, uint8_t topicType){
	Topic* p = _first;
	while(p){
		if (p->_topicId == topicId && p->_topicType == topicType){
			return p;
		}
		p = p->_next;
	}
	return 0;
}

uint16_t TopicTable::getTopicId(const char* topic){
	Topic* p = getTopic(topic);
	if (p){
		return p->_topicId;
	}
	return 0;
}


const char* TopicTable::getTopicName(Topic* topic){
	return topic->_topicStr;
}


void TopicTable::setTopicId(const char* topic, uint16_t id, uint8_t type){
    Topic* tp = getTopic(topic);
    if (tp){
        tp->_topicId = id;
    }else{
    	add(topic, id, type, 0);
    }
}


bool TopicTable::setCallback(const char* topic, TopicCallback callback){
	Topic* p = getTopic(topic);
	if (p){
		p->_callback = callback;
		return true;
	}
	return false;
}


bool TopicTable::setCallback(uint16_t topicId, uint8_t topicType, TopicCallback callback){
	Topic* p = getTopic(topicId, topicType);
	if (p){
		p->_callback = callback;
		return true;
	}
	return false;
}


int TopicTable::execCallback(uint16_t  topicId, uint8_t* payload, uint16_t payloadlen, uint8_t topicType){
	Topic* p = getTopic(topicId, topicType);
	if (p){;
		return p->execCallback(payload, payloadlen);
	}
	return 0;
}


Topic* TopicTable::add(const char* topicName, uint16_t id, uint8_t type, TopicCallback callback, uint8_t alocFlg){
	Topic* last = _first;
	Topic* prev = _first;
	Topic* elm;

    if (topicName){
	    elm = getTopic(topicName);
    }else{
        elm = getTopic(id, type);
    }

	if (elm == 0){
		elm = new Topic();
		if(elm == 0){
			return 0;
		}
		if ( last == 0){
			_first = elm;
			_last = elm;
		}
		elm->_topicStr =  const_cast <char*>(topicName);
		elm->_topicId = id;
        elm->_topicType = type;
		elm->_callback = callback;
		elm->_malocFlg = alocFlg;
		elm->_prev = 0;

		while(last){
			prev = last;
			if(prev->_next != 0){
				last = prev->_next;
			}else{
				prev->_next = elm;
				elm->_prev = prev;
				elm->_next = 0;
				last = 0;
				_last = elm;
			}
		}
	}else{
		elm->_callback = callback;
	}
	return elm;
}

void TopicTable::remove(uint16_t topicId)
{
	Topic* elm = getTopic(topicId);

	if (elm){
		if (elm->_prev == 0){
			_first = elm->_next;
			if (elm->_next != 0){
				elm->_next->_prev = 0;
			}
			else
			{
				_last = elm;
			}
			delete elm;
		}else{
			elm->_prev->_next = elm->_next;
			if (elm->_next == 0)
			{
				_last = elm;
			}
			delete elm;
		}
	}
}

Topic* TopicTable::match(const char* topicName){
	Topic* elm = _first;
	while(elm){
		if (elm->isMatch(topicName)){
			return elm;
		}
		elm = elm->_next;
	}
	return 0;
}


void TopicTable::clearTopic(void){
	Topic* p = _first;
	while(p){
		_first = p->_next;
		delete p;
		p = _first;
	}
	_last = 0;
}
