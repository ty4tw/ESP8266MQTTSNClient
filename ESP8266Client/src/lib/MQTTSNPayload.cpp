/*
 * MqttsnClient.cpp.cpp
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
#include <MQTTSNPayload.h>
#include <stdlib.h>
#include <string.h>

using namespace std;
using namespace ESP8266MQTTSNClient;

extern uint16_t getUint16(const uint8_t* pos);
extern uint32_t getUint32(const uint8_t* pos);
extern float    getFloat32(const uint8_t* pos);

extern void setUint16(uint8_t* pos, uint16_t val);
extern void setUint32(uint8_t* pos, uint32_t val);
extern void setFloat32(uint8_t* pos, float val);

/*=====================================
        Class MQTTSNPayload
  =====================================*/
MQTTSNPayload::MQTTSNPayload(){
	_buff = _pos = 0;
	_len = 0;
	_elmCnt = 0;
	_memDlt = 0;
}

MQTTSNPayload::MQTTSNPayload(uint16_t len){
	_buff = (uint8_t*)calloc(len, sizeof(uint8_t));
	if(_buff == 0){
		// ToDo
	}
	_pos = _buff;
	_elmCnt = 0;
	_len = len;
	_memDlt = 1;
}

MQTTSNPayload::~MQTTSNPayload(){
	if(_memDlt){
		free(_buff);
	}
}

void MQTTSNPayload::init(){
	_pos = _buff;
	_elmCnt = 0;
}

uint16_t MQTTSNPayload::getAvailableLength(){
	return _len - (_pos - _buff);
}

uint16_t MQTTSNPayload::getLen(){
	return _pos - _buff;
}

uint8_t* MQTTSNPayload::getRowData(){
	return _buff;
}

/*======================
 *     setter
 ======================*/
int8_t MQTTSNPayload::set_uint32(uint32_t val){
	if(getAvailableLength() < 6){
		return -1;
	}
	if(val < 128){
		*_pos++ = (uint8_t)val;
	}else if(val < 256){
		*_pos++ = MSGPACK_UINT8;
		*_pos++ = (uint8_t)val;
	}else if(val < 65536){
		*_pos++ = MSGPACK_UINT16;
		setUint16(_pos,(uint16_t) val);
		_pos += 2;
	}else{
		*_pos++ = MSGPACK_UINT32;
		setUint32(_pos, val);
		_pos += 4;
	}
	_elmCnt++;
	return 0;
}

int8_t MQTTSNPayload::set_int32(int32_t val){
	if(getAvailableLength() < 6){
			return -1;
	}
	if((val > -32) && (val < 0)){
		*_pos++ = val | MSGPACK_NEGINT;
	}else if((val >= 0) && (val < 128)){
		*_pos++ = val;
	}else if(val > -128 && val < 128){
		*_pos++ = MSGPACK_INT8;
		*_pos++ = (uint8_t)val;
	}else if(val > -32768 && val < 32768){
		*_pos++ = MSGPACK_INT16;
		setUint16(_pos, (uint16_t)val);
		_pos += 2;
	}else{
		*_pos++ = MSGPACK_INT32;
		setUint32(_pos, (uint32_t)val);
		_pos += 4;
	}
	_elmCnt++;
	return 0;
}

int8_t MQTTSNPayload::set_float(float val){
	if(getAvailableLength() < 6){
			return -1;
	}
	*_pos++ = MSGPACK_FLOAT32;
	setFloat32(_pos, val);
	_pos += 4;
	_elmCnt++;
	return 0;
}

int8_t MQTTSNPayload::set_str(char* val){
	return set_str((const char*) val);
}

int8_t MQTTSNPayload::set_str(const char* val){
	if(getAvailableLength() < strlen(val) + 3){
		return -1;
	}else if(strlen(val) < 32){
		*_pos++ = (uint8_t)strlen(val) | MSGPACK_FIXSTR;
	}else if(strlen(val) < 256){
		*_pos++ = MSGPACK_STR8;
		*_pos++ = (uint8_t)strlen(val);
	}else if(strlen(val) < 65536){
		*_pos++ = MSGPACK_STR16;
		setUint16(_pos, (uint16_t)strlen(val));
		_pos += 2;
	}
	memcpy(_pos, val, strlen(val));
	_pos += strlen(val);
	return 0;
}

int8_t MQTTSNPayload::set_array(uint8_t val){
	if(getAvailableLength() < (uint16_t)val+ 1){
		return -1;
	}
	if(val < 16){
		*_pos++ = MSGPACK_ARRAY15 | val;
	}else{
		*_pos++ = MSGPACK_ARRAY16;
		setUint16(_pos,(uint16_t)val);
		_pos += 2;
	}
	_elmCnt++;
	return 0;
}

int8_t MQTTSNPayload::set_bool(bool val){
	if (getAvailableLength() < 1){
			return -1;
	}
	if (val){
		*_pos++ = MSGPACK_TRUE;
	}else {
		*_pos++ = MSGPACK_FALSE;
	}
	_elmCnt++;
	return 0;
}
/*======================
 *     getter
 ======================*/
uint8_t MQTTSNPayload::getArray(uint8_t index){
	uint8_t rc = 0;
	uint8_t* val = getBufferPos(index);
	if(val != 0){
		if(*val == MSGPACK_ARRAY15){
			rc = *val & 0x0F;
		}else if(*val == MSGPACK_ARRAY16){
			rc = (uint8_t)getUint16(val + 1);
		}
	}
	return rc;
}

bool MQTTSNPayload::get_bool(uint8_t index){
	uint8_t* val = getBufferPos(index);
	if (*val == MSGPACK_FALSE){
		return false;
	}else{
		return true;
	}
}

uint32_t MQTTSNPayload::get_uint32(uint8_t index){
	uint32_t rc = 0;
	uint8_t* val = getBufferPos(index);
	if(val != 0){
		if(*val == MSGPACK_UINT32){
			rc = getUint32(val + 1);
		}else if(*val == MSGPACK_UINT16){
			rc = (uint32_t)getUint16(val + 1);
		}else if(*val == MSGPACK_UINT8){
			rc = (uint32_t)*(val + 1);
		}else if(*val < 128){
			rc = (uint32_t)*val;
		}
	}
	return rc;
}

int32_t MQTTSNPayload::get_int32(uint8_t index){
	int32_t rc = 0;
	uint8_t* val = getBufferPos(index);
	if(val != 0){
		if(*val == MSGPACK_INT32){
			rc = (int32_t) getUint32(val + 1);
		}else if(*val == MSGPACK_INT16){
			uint16_t d16 = getUint16(val + 1);
			if(d16 >= 32768){
				rc = d16 - 65536;
			}else{
				rc = (int32_t)d16;
			}
		}else if(*val == MSGPACK_INT8){
			rc = (int32_t)*(val + 1);
		}else if((*val & MSGPACK_NEGINT) == MSGPACK_NEGINT){
			*val &= ~MSGPACK_NEGINT;
			rc = ((int32_t)*val) * -1;
		}else{
			rc = (int32_t) *val;
		}
	}
	return rc;
}

float MQTTSNPayload::get_float(uint8_t index){
	uint8_t* val = getBufferPos(index);
	if(val != 0){
		if(*val == MSGPACK_FLOAT32){
			return getFloat32(val + 1);
		}
	}
	return 0;
}

const char* MQTTSNPayload::get_str(uint8_t index, uint16_t* len){
	uint8_t* val = getBufferPos(index);
	if(val != 0){
		if(*val == MSGPACK_STR16){
			*len = getUint16(val + 1);
			return (const char*)(val + 3);
		}else if(*val == MSGPACK_STR8){
			*len = *(val + 1);
			return (const char*)(val + 2);
		}else if(*val & MSGPACK_FIXSTR){
			*len = *val & (~MSGPACK_FIXSTR);
			return (const char*)(val + 1);
		}
	}
	*len = 0;
	return (const char*) 0;

}


uint8_t* MQTTSNPayload::getBufferPos(uint8_t index){
	uint8_t* bpos = 0;
	uint8_t* pos = _buff;

	for(uint8_t i = 0; i <= index; i++){
		bpos = pos;
		switch(*pos){
		case MSGPACK_FALSE:
		case MSGPACK_TRUE:
			pos++;
			break;
		case MSGPACK_UINT8:
		case MSGPACK_INT8:
			pos += 2;
			break;
		case MSGPACK_UINT16:
		case MSGPACK_INT16:
		case MSGPACK_ARRAY16:
			pos += 3;
			break;
		case MSGPACK_UINT32:
		case MSGPACK_INT32:
		case MSGPACK_FLOAT32:
			pos += 5;
			break;
		case MSGPACK_STR8:
			pos += *(pos + 1) + 2;
			break;
		case MSGPACK_STR16:
			pos += getUint16(pos + 1) + 3;
			break;
		default:
			if((*pos < MSGPACK_POSINT) ||
				((*pos & MSGPACK_NEGINT) == MSGPACK_NEGINT) ||
				((*pos & MSGPACK_ARRAY15) == MSGPACK_ARRAY15)) {
				pos++;
			}else if((*pos & MSGPACK_FIXSTR) == MSGPACK_FIXSTR){
				pos += *pos & (~MSGPACK_FIXSTR);
			}
		}
		/*
		if((pos - _buff) >= _len){
			return 0;
		}
		*/
	}
	return bpos;
}

void MQTTSNPayload::getPayload(uint8_t* payload, uint16_t payloadLen){
    if(_memDlt){
			free(_buff);
			_memDlt = 0;
	}
	_buff = payload;
	_len = payloadLen;
	_pos = _buff + _len;
}



