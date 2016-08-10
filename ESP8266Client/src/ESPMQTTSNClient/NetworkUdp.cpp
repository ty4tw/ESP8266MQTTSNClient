/*
 * NetworkUDP.cpp
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
#include <NetworkUdp.h>
#include <Timer.h>

using namespace std;
using namespace ESP8266MQTTSNClient;

extern uint16_t getUint16(const uint8_t* pos);
extern uint32_t getUint32(const uint8_t* pos);
extern const char* theSsid;
extern const char* thePasswd;

/*=========================================
       Class Network
 =========================================*/
Network::Network(){
	_sleepflg = false;
	resetGwAddress();
}

Network::~Network(){

}

int Network::broadcast(const uint8_t* xmitData, uint16_t dataLen){
	return UdpPort::multicast(xmitData, (uint32_t)dataLen);
}

int  Network::unicast(const uint8_t* xmitData, uint16_t dataLen){
	return UdpPort::unicast(xmitData, dataLen, _gwIpAddress, _gwPortNo);
}


uint8_t*  Network::getMessage(int* len){
	*len = 0;
	if (checkRecvBuf()){
		D_NWLOG("Network::getMessage() data received..\n");
		uint16_t recvLen = UdpPort::recv(_rxDataBuf, MQTTSN_MAX_PACKET_SIZE, false, &_ipAddress, &_portNo);
		if(_gwIpAddress && isUnicast() && (_ipAddress != _gwIpAddress) && (_portNo != _gwPortNo))
		{
			return 0;
		}

		if(recvLen < 0){
			*len = recvLen;
			return 0;
		}else{
			if(_rxDataBuf[0] == 0x01){
				*len = getUint16(_rxDataBuf + 1 );
			}else{
				*len = _rxDataBuf[0];
			}
			if(recvLen != *len){
				*len = 0;
				return 0;
			}else{
				return _rxDataBuf;
			}
		}
	}
	return 0;
}

void Network::setGwAddress(void){
	_gwPortNo = _portNo;
	_gwIpAddress = _ipAddress;
}

void Network::setFixedGwAddress(void){
    _gwPortNo = UdpPort::_gPortNo;
    _gwIpAddress = UdpPort::_gIpAddr;
}

void Network::resetGwAddress(void){
	_gwIpAddress = 0;
	_gwPortNo = 0;
}


bool Network::open(NETCONF  config){
	return UdpPort::open(config);
}

void Network::close(void)
{
	UdpPort::close();
}

void Network::setSleep(){
	_sleepflg = true;
}

/*=========================================
       Class udpStack
 =========================================*/
UdpPort::UdpPort(){

}

UdpPort::~UdpPort(){
    close();
}

void UdpPort::close(){
	_udpMulticast.stop();
	_udpMulticast.stop();
}


bool UdpPort::open(UdpConfig config){
	_gIpAddr = IPAddress(config.ipAddress);
	_gPortNo = config.gPortNo;
	_uPortNo = config.uPortNo;


	if ( WiFi.status() != WL_CONNECTED)
	{
		D_NWLOG("UdpPort::WiFi Attempting to connect.\n");
		WiFi.mode(WIFI_STA);
		WiFi.begin(theSsid, thePasswd);
	}

	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		D_NWLOG(".");
	}
	D_NWLOG("UdpPort::WiFi Connected\n");

	_cIpAddr = WiFi.localIP();

	if(_udpMulticast.beginMulticast(_cIpAddr, _gIpAddr, _gPortNo) == 0){
		return false;
	}
	if(_udpUnicast.begin(_uPortNo) == 0){
		return false;
	}

	return true;
}

int UdpPort::unicast(const uint8_t* buf, uint32_t length, uint32_t ipAddress, uint16_t port  ){

	IPAddress ip = IPAddress(ipAddress);
	_udpUnicast.beginPacket(ip, port);
	_udpUnicast.write(buf, length);
	int status = _udpUnicast.endPacket();

#ifdef DEBUG_NW
	if( status == 1){
		D_NWLOG("UNICAST [ ");
		for(uint16_t i = 0; i < length ; i++){
			D_NWLOG("%02x ",*(buf + i));
		}
		D_NWLOG("]\n");
	}
#endif

	return status;
}


int UdpPort::multicast( const uint8_t* buf, uint32_t length ){
	_udpMulticast.beginPacket(_gIpAddr, _gPortNo);
	_udpMulticast.write(buf, length);
	int status = _udpMulticast.endPacket();

#ifdef DEBUG_NW
	if( status == 1 ){
		D_NWLOG("MULCAST [ ");
		for(uint16_t i = 0; i < length ; i++){
			D_NWLOG("%02x ",*(buf + i));
		}
		D_NWLOG("]\n");
	}
#endif

	return status;
}

bool UdpPort::checkRecvBuf(){
	int ps;
	if( (ps = _udpUnicast.parsePacket()) > 0){
		_castStat = STAT_UNICAST;
		return true;
	}else if ( (ps = _udpMulticast.parsePacket()) > 0){
		_castStat = STAT_MULTICAST;
		return true;
	}
	_castStat = 0;
	return false;
}

int UdpPort::recv(uint8_t* buf, uint16_t len, bool flg, uint32_t* ipAddressPtr, uint16_t* portPtr){
	return recvfrom ( buf, len, 0, ipAddressPtr, portPtr );
}

int UdpPort::recvfrom ( uint8_t* buf, uint16_t len, int flags, uint32_t* ipAddressPtr, uint16_t* portPtr ){
	IPAddress remoteIp;
	uint8_t packLen;
	if(_castStat == STAT_UNICAST){
		packLen = _udpUnicast.read(buf, len);
		*portPtr = _udpUnicast.remotePort();
		remoteIp = _udpUnicast.remoteIP();
	}else if(_castStat == STAT_MULTICAST){
		packLen = _udpMulticast.read(buf, len);
		*portPtr = _udpMulticast.remotePort();
		remoteIp = _udpMulticast.remoteIP();
	}else{
		return 0;
	}

	*ipAddressPtr = (const uint32_t)remoteIp;

#ifdef DEBUG_NW
	D_NWLOG("RECIEVE [ ");
	for(uint16_t i = 0; i < packLen ; i++){
		D_NWLOG("%02x ",*(buf + i));
	}
	D_NWLOG("]\n");
#endif

	return packLen;
}

bool UdpPort::isUnicast(){
	return ( _castStat == STAT_UNICAST);
}




