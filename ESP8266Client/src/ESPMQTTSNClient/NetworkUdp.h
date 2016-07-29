/*
 * NetworkUDP.h
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

#ifndef NETWORKUDP_H_
#define NETWORKUDP_H_

#include <inttypes.h>
#include <MqttsnClientApp.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define SOCKET_MAXHOSTNAME  200
#define SOCKET_MAXCONNECTIONS  5
#define SOCKET_MAXRECV  500
#define SOCKET_MAXBUFFER_LENGTH 500 // buffer size

//#define PACKET_TIMEOUT_CHECK   200  // msec

#define STAT_UNICAST   1
#define STAT_MULTICAST 2

using namespace std;

namespace ESP8266MQTTSNClient {
/*========================================
       Class UpdPort
 =======================================*/
class UdpPort{
    friend class Network;
public:
	UdpPort();
	virtual ~UdpPort();

	bool open(UdpConfig config);

	int unicast(const uint8_t* buf, uint32_t length, uint32_t ipaddress, uint16_t port  );
	int multicast( const uint8_t* buf, uint32_t length );
	int recv(uint8_t* buf, uint16_t len, bool nonblock, uint32_t* ipaddress, uint16_t* port );
	int recv(uint8_t* buf, int flags);
	bool checkRecvBuf(void);
	bool isUnicast(void);
	void close(void);
private:
	int recvfrom ( uint8_t* buf, uint16_t len, int flags, uint32_t* ipaddress, uint16_t* port );
	WiFiUDP    _udpUnicast;
	WiFiUDP    _udpMulticast;
	IPAddress  _gIpAddr;
	IPAddress  _cIpAddr;
	uint16_t   _gPortNo;
	uint16_t   _uPortNo;
	uint8_t*   _macAddr;
	uint8_t    _castStat;
	bool   _disconReq;

};

#define NO_ERROR	0
#define PACKET_EXCEEDS_LENGTH  1
/*===========================================
               Class  Network
 ============================================*/
class Network : public UdpPort {
public:
    Network();
    ~Network();

    int  broadcast(const uint8_t* payload, uint16_t payloadLen);
    int  unicast(const uint8_t* payload, uint16_t payloadLen);
    void setGwAddress(void);
    void resetGwAddress(void);
    void setFixedGwAddress(void);
    bool open(NETCONF  config);
    void close(void);
    uint8_t*  getMessage(int* len);
private:
    void setSleep();
    int  readApiFrame(void);

    uint32_t _gwIpAddress;
	uint16_t _gwPortNo;
	uint32_t _ipAddress;
	uint16_t _portNo;
    int     _returnCode;
    bool    _sleepflg;
    uint8_t _rxDataBuf[MQTTSN_MAX_PACKET_SIZE + 1];  // defined in MqttsnClientApp.h

};


}    /* end of namespace */

#endif /* NETWORKUDP_H_ */
