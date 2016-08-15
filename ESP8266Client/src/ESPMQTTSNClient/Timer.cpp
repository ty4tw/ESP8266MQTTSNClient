/*
 * Timer.cpp
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

#include "MqttsnClientApp.h"
#include "Timer.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/reent.h>
#include <sntp.h>

using namespace std;
using namespace ESP8266MQTTSNClient;

/*=====================================
        Class Timer
 =====================================*/
time_t Timer::_unixTime = 0;
time_t Timer::_epochTime = 0;
uint32_t Timer::_timerStopTimeAccum = 0;
uint32_t Timer::_updateInterval = 0;
uint32_t Timer::_init = 0;

Timer::Timer(){
    stop();
}

void Timer::start(uint32_t msec){
    _startTime = millis();
    _millis = msec;
    _currentTime = 0;
}

bool Timer::isTimeUp(){
    return isTimeUp(_millis);
}

bool Timer::isTimeUp(uint32_t msec){
	if ( _startTime){
		_currentTime = millis();
		if ( _currentTime < _startTime){
			return (0xffffffff - _startTime + _currentTime > msec);
		}else{
			return (_currentTime - _startTime > msec);
		}
	}else{
		return false;
	}
}

void Timer::stop(){
    _startTime = 0;
    _millis = 0;
    _currentTime = 0;
}

void Timer::setStopTimeDuration(uint32_t msec){
	_timerStopTimeAccum += msec;
}


void Timer::initialize(int32 timezone, uint32_t daylightOffset_sec, const char* server1,
		const char* server2, const char* server3, uint32_t interval)
{
	configTime(timezone * 3600, daylightOffset_sec, server1, server2, server3);
	_updateInterval = interval * 1000;
}

bool Timer::update(void)
{
	 uint32_t tm = _timerStopTimeAccum + millis();
	 uint32_t interval;

	if (_epochTime > tm ){
	    interval = 0xffffffff - tm - _epochTime;
	}
	else
	{
		interval = tm - _epochTime;
	}

	if ( interval >= _updateInterval || _init == 0 )
	{
		uint8_t timeout = 0;
		time_t tm = 0;
		do
		{
			delay(10);
			tm = time(NULL);
			if ( timeout > 100 )
			{
				return false;
			}
			timeout++;
		}
		while ( tm == 0 );
		_unixTime = tm;
		_epochTime = millis();
		_timerStopTimeAccum = 0;
		_init = 1;
	}
	return true;
}

const char* Timer::getNow(void)
{
	struct tm* time_inf;
	time_t timep;
	timep = time(NULL);
	time_inf = localtime(&timep);
	return asctime(time_inf);
}

