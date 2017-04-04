#ifndef _SCMPACKET_H_
#define _SCMPACKET_H_

#include "SCMProtocol.h"

#define MAX_SCM_PACKETS 50

typedef struct 
{
	unsigned int closedPacketNum;
	unsigned long long closedIndexes[MAX_SCM_PACKETS];
	unsigned long long closedLengths[MAX_SCM_PACKETS];
	bool allBytesClosed;
}AnalyseReturn;

namespace SCMProtocol
{
	class SCMPacket
	{
	public:
		SCMPacket(); //No Payload No packet
		SCMPacket(PeerType _type, unsigned long long _id, Command cmd, unsigned char _channel, unsigned char * in_payload, unsigned long long pload_len);
		SCMPacket(unsigned char * in_packet, unsigned long long packet_len);
		~SCMPacket();

		bool setHeader(PeerType _type, unsigned long long _id, Command cmd, unsigned char _channel);
		bool setPeerSnitch();
		bool setPeerServer();
		bool setPeerType(PeerType type);
		bool setPeerId(unsigned long long id);
		bool setCommand(Command cmd);
		bool setChannel(unsigned char _channel);
		bool setPacket(unsigned char * packet, unsigned long long packet_len);
		bool setPayload(unsigned char * payl, unsigned long long payl_len);
		bool clearPayload();

		PeerType getPeerType();
		unsigned long long getPeerId();
		Command getCommand();
		unsigned char getChannel();
		unsigned char* getPayload(unsigned long long* out_len); //Returns a new array with the pay load, Be careful with memory leak
		unsigned char* getPacket(unsigned long long* out_len); // the same

		bool isValid();

		static bool analysePacket(SCMProtocol::PeerType matchType, unsigned char* in_packet, unsigned long long in_packetLen, AnalyseReturn * out_ret);
		static bool isHeader(unsigned char* in_header, unsigned int size, unsigned long long * out_payloadSize);

	private:
		unsigned char startKey[4];
		unsigned char type;
		union
		{
			unsigned long long id;
			unsigned char idBytes[8];
		};
		unsigned char command;
		unsigned char channel;
		union
		{
			unsigned long long len;
			unsigned char lenBytes[8];
		};
		unsigned char* payload;
		unsigned char endKey[4];


		bool isHeaderValid();

	};
}

#endif