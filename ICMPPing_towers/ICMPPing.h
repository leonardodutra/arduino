/*
 * Copyright (c) 2010 by Blake Foster <blfoster@vassar.edu>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#include <SPI.h>
#include <Ethernet.h>
#include <utility/w5100.h>

#define ICMP_ECHOREPLY 0
#define ICMP_ECHOREQ 8
#define PING_TIMEOUT 1000

static int lastSeq = 0;
static int lastId = 0;

typedef unsigned long time_t;

template <int requestDatasize>
class ICMPHeader;
template <int dataSize>
class ICMPMessage;

template <int requestDatasize>
class ICMPPing
{
public:
	typedef ICMPMessage<requestDatasize> EchoRequest;
	typedef ICMPMessage<requestDatasize> EchoReply;

	ICMPPing(SOCKET s): socket(s)
	{}
	bool operator()(int nRetries, byte * addr, char * result)
	{
		W5100.execCmdSn(socket, Sock_CLOSE);
		W5100.writeSnIR(socket, 0xFF);
		W5100.writeSnMR(socket, SnMR::IPRAW);
		W5100.writeSnPROTO(socket, IPPROTO::ICMP);
		W5100.writeSnPORT(socket, 0);
	    W5100.execCmdSn(socket, Sock_OPEN);
		bool rval = false;
		for (int i=0; i<nRetries; ++i)
		{
			if (sendEchoRequest(addr) < 0)
			{
				sprintf(result, "sendEchoRequest failed.");
				break;
			}
			if (waitForEchoReply())
			{
				uint8_t TTL;
				byte replyAddr [4];
				time_t timeSent;
				receiveEchoReply(replyAddr, TTL, timeSent);
				time_t time = millis() - timeSent;
				sprintf(result, "Reply[%d] from: %d.%d.%d.%d: bytes=%d time=%ldms TTL=%d", i + 1, replyAddr[0], replyAddr[1], replyAddr[2], replyAddr[3], requestDatasize, time, TTL);
				rval = true;
				break;
			}
			else
			{
				sprintf(result, "Request Timed Out");
			}
		}
		W5100.execCmdSn(socket, Sock_CLOSE);
		W5100.writeSnIR(socket, 0xFF);
		return rval;
	}
private:
	bool waitForEchoReply() {
		time_t start = millis();
		while (!W5100.getRXReceivedSize(socket))
		{
			if (millis() - start > PING_TIMEOUT) return false;
		}
		return true;
	};

	size_t sendEchoRequest(byte * addr)
	{
		EchoRequest echoReq(ICMP_ECHOREQ);
		for (int i = 0; i < requestDatasize; i++) echoReq[i] = ' ' + i;
		echoReq.initChecksum();
	    W5100.writeSnDIPR(socket, addr);
	    W5100.writeSnDPORT(socket, 0);
	    W5100.send_data_processing(socket, (uint8_t *)&echoReq, sizeof(EchoRequest));
	    W5100.execCmdSn(socket, Sock_SEND);
	    while ((W5100.readSnIR(socket) & SnIR::SEND_OK) != SnIR::SEND_OK) 
	    {
			if (W5100.readSnIR(socket) & SnIR::TIMEOUT)
			{
				W5100.writeSnIR(socket, (SnIR::SEND_OK | SnIR::TIMEOUT));
				return 0;
			}
	    }
	    W5100.writeSnIR(socket, SnIR::SEND_OK);
		return sizeof(EchoRequest);
	}

	uint8_t receiveEchoReply(byte * addr, uint8_t& TTL, time_t& time)
	{
		EchoReply echoReply;
		uint16_t port = 0;
		uint8_t header [6];
		uint8_t buffer = W5100.readSnRX_RD(socket);
		W5100.read_data(socket, (uint8_t *)buffer, header, sizeof(header));
		buffer += sizeof(header);
		for (int i=0; i<4; ++i) addr[i] = header[i];
		uint8_t dataLen = header[4];
		dataLen = (dataLen << 8) + header[5];
		if (dataLen > sizeof(EchoReply)) dataLen = sizeof(EchoReply);
		W5100.read_data(socket, (uint8_t *)buffer, (uint8_t *)&echoReply, dataLen);
		buffer += dataLen;
		W5100.writeSnRX_RD(socket, buffer);
		W5100.execCmdSn(socket, Sock_RECV);
		TTL = W5100.readSnTTL(socket);
		time = echoReply.time;
		return echoReply.icmpHeader.type;
	}
	SOCKET socket; // socket number to send ping
};

template <int requestDatasize>
class ICMPHeader
{
	friend class ICMPPing<requestDatasize>;
public:
	ICMPHeader(uint8_t Type): type(Type), code(0), checksum(0), seq(++lastSeq), id(++lastId)
	{}
	ICMPHeader(): type(0), code(0), checksum(0)
	{}
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint16_t id;
	uint16_t seq;
};

template <int dataSize>
class ICMPMessage
{
	friend class ICMPPing<dataSize>;
public:
	ICMPMessage(uint8_t type): icmpHeader(type), time(millis())
	{}
	ICMPMessage(): icmpHeader()
	{}
	void initChecksum()
	{
		icmpHeader.checksum = 0;
		int nleft = sizeof(ICMPMessage<dataSize>);
		uint16_t * w = (uint16_t *)this;
		unsigned long sum = 0;
		while(nleft > 1)  
		{
			sum += *w++;
			nleft -= 2;
		}
		if(nleft)
		{
			uint16_t u = 0;
			*(uint8_t *)(&u) = *(uint8_t *)w;
			sum += u;
		}
		sum = (sum >> 16) + (sum & 0xffff);
		sum += (sum >> 16);
		icmpHeader.checksum = ~sum;
	}

	uint8_t& operator[](int i);
	const uint8_t& operator[](int i) const;
	ICMPHeader<dataSize> icmpHeader;
	time_t time;
private:
	uint8_t data [dataSize];
};

template <int dataSize>
inline uint8_t& ICMPMessage<dataSize>::operator[](int i)
{
	return data[i];
}

template <int dataSize>
inline const uint8_t& ICMPMessage<dataSize>::operator[](int i) const
{
	return data[i];
}


#pragma pack(1)