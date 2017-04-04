#include "SCMPacket.h"
#include "Exception.h"
#include <iostream>

SCMProtocol::SCMPacket::SCMPacket()
{
	int i=0;

	for(i=0;i<4;i++)
	{
		startKey[i] = SCMProtocol::SCMStartKey[i];
		endKey[i] = SCMProtocol::SCMEndKey[i];
	}
	this->type=InvalidType;
	this->id=0;
	this->command=InvalidCommand;
	this->channel = 0;
	this->len = 0;
	this->payload = 0;
}

SCMProtocol::SCMPacket::SCMPacket(PeerType _type, unsigned long long _id, Command cmd, unsigned char _channel, unsigned char * in_payload, unsigned long long pload_len)
{
	int i=0;

	for(i=0;i<4;i++)
	{
		startKey[i] = SCMProtocol::SCMStartKey[i];
		endKey[i] = SCMProtocol::SCMEndKey[i];
	}
	this->type=InvalidType;
	this->id=0;
	this->command=InvalidCommand;
	this->channel = 0;
	this->len = 0;
	this->payload = 0;

	if(!setHeader(_type,_id,cmd,_channel))
	{
		throw Exception("Invalid Header.");
	}
	
	setPayload(in_payload,pload_len);
}

SCMProtocol::SCMPacket::SCMPacket(unsigned char * in_packet, unsigned long long packet_len)
{
	if(!in_packet || packet_len<SCMProtocol::SCMStartHeaderSize)
	{
		throw Exception("Invalid Packet or too short.");
	}

	this->type=InvalidType;
	this->id=0;
	this->command=InvalidCommand;
	this->channel = 0;
	this->len = 0;
	this->payload = 0;

	if(!setPacket(in_packet,packet_len))
	{
		throw Exception("Invalid Packet, couldn't be parsed.");
	}
}

SCMProtocol::SCMPacket::~SCMPacket()
{
	if(payload)
	{
		delete[] payload;
		payload=0;
	}
}

bool SCMProtocol::SCMPacket::analysePacket(SCMProtocol::PeerType matchType, unsigned char* in_packet, unsigned long long in_packetLen, AnalyseReturn * out_ret)
{
	unsigned long long i=0,k=0,j=0,m=0;
	bool startFlag=false;
	bool stopFlag=false;

	unsigned long long payLen=0;
	unsigned long long parsedLen=0;

	if(!in_packet || in_packetLen<4 || !out_ret)
	{
		return false;
	}

	m=0;
	for(i=0;i<in_packetLen;i=i+4)
	{
		startFlag=false;
		stopFlag=false;

		for(j=0;j<4;j++)
		{
			if(in_packet[i]==SCMProtocol::SCMStartKey[j])
			{
				startFlag=true;
				break;
			}
		}

		if(startFlag)
		{
			if(i>=j)
			{
				k=i-j;
				if((k+3)>in_packetLen)
				{
					return false;
				}
				for(j=0;j<4;j++)
				{
					if(in_packet[k+j]!=SCMProtocol::SCMStartKey[j])
					{
						startFlag=false;
						break;
					}
				}
			}
			else
			{
				startFlag=false;
			}

			if(startFlag)
			{
				if((k+SCMProtocol::SCMStartHeaderSize) > in_packetLen)
				{
					//Outside packet boudaries, waiting append
					out_ret->closedPacketNum=m;
					out_ret->allBytesClosed=false;
					return false;
				}
				else if(isHeader(&in_packet[k],SCMProtocol::SCMStartHeaderSize,&payLen))
				{
					/*if(in_packet[k+5]!=matchType)
					{
						std::cout << "Didn't Match Type" << std::endl;
						continue;
					}*/

					if((k+SCMStartHeaderSize+payLen+4)>in_packetLen)
					{
						//Outside packet boudaries, waiting append
						/*out_ret->closedPacketNum=m;
						out_ret->allBytesClosed=false;
						return false;*/
						continue;
					}

					stopFlag=true;

					for(j=0;j<4;j++)
					{
						if(in_packet[k+SCMProtocol::SCMStartHeaderSize+payLen+j]!=SCMProtocol::SCMEndKey[j])
						{
							stopFlag=false;
							break;
						}
					}
					if(stopFlag)
					{
						//Closed Packet at K
						out_ret->closedIndexes[m]=k;
						out_ret->closedLengths[m]=SCMProtocol::SCMHeaderSize+payLen;
						parsedLen+=SCMProtocol::SCMHeaderSize+payLen;
						m++;
						i=k+SCMHeaderSize+payLen-4;
						stopFlag=false;
						if(m>MAX_SCM_PACKETS)
						{
							break; //Ignoring extra packets outside SCM boundaries.
						}
					}
					else
					{
						//Not a valid packet
					}
				}
				else
				{
					std::cout << "Couldn't parse header" << std::endl;
					//not a valid packet
				}
			}
		}
	}

	out_ret->closedPacketNum=m;
	out_ret->allBytesClosed=(parsedLen==in_packetLen);
	return (parsedLen==in_packetLen);
}

bool SCMProtocol::SCMPacket::isHeader(unsigned char* in_header, unsigned int size, unsigned long long* out_payloadSize)
{
	if(size!=SCMProtocol::SCMStartHeaderSize)
	{
		return false;
	}
	
	union
	{
		unsigned long long out_len;
		unsigned char out_lenBytes[8];
	};
	
	unsigned int i=0;
	out_len=0;

	for(i=0;i<SCMProtocol::SCMStartHeaderSize;i++)
	{
		if(i<4)
		{
			if(in_header[i] != SCMProtocol::SCMStartKey[i])
			{
				return false;
			}
		}
		else if(i<5)
		{
			if(in_header[i]!=SCMProtocol::Server && in_header[i]!=SCMProtocol::Client)
			{
				return false;
			}
		}
		else if(i<13)
		{
			//
		}
		else if(i<14)
		{
			if(in_header[i]<SCMProtocol::Register || in_header[i]>SCMProtocol::HeartBeatAck)
			{
				return false;
			}
		}
		else if(i<15)
		{
			if(in_header[i]>=SCM_MAX_CHANNELS)
			{
				return false;
			}
		}
		else if(i<SCMProtocol::SCMStartHeaderSize)
		{
			out_lenBytes[i-15] = in_header[i];
		}
		else
		{
			std::cout << "SCMPacket: Internal Error.." << std::endl;
			return false;
		}
	}
	*out_payloadSize = out_len;
	return true;
}

bool SCMProtocol::SCMPacket::setPacket(unsigned char * packet, unsigned long long packet_len)
{
	if(!packet || SCMHeaderSize<SCMProtocol::SCMHeaderSize)
	{
		std::cout << "SCMPacket: Invalid Packet." << std::endl;
		return false;
	}
	
	unsigned long long i=0;
	for(i=0;i<SCMProtocol::SCMStartHeaderSize;i++)
	{
		if(i<4)
		{
			startKey[i] = packet[i];
		}
		else if(i<5)
		{
			type = packet[i];
		}
		else if(i<13)
		{
			idBytes[i-5] = packet[i];
		}
		else if(i<14)
		{
			command = packet[i];
		}
		else if(i<15)
		{
			channel = packet[i];
		}
		else if(i<SCMProtocol::SCMStartHeaderSize)
		{
			lenBytes[i-15] = packet[i];
		}
		else
		{
			std::cout << "SCMPacket: Internal Error." << std::endl;
			return false;
		}
	}

	if((len+SCMProtocol::SCMHeaderSize) != packet_len)
	{
		std::cout << "SCMPacket: Invalid Length." << std::endl;
		std::cout << "SCMPacket: len " << (unsigned long long)len << std::endl;
		std::cout << "SCMPacket: packet_len " << (unsigned long long)packet_len << std::endl;

		for(i=0;i<packet_len;i++)
		{
			std::cout << "SCMPacket: Packet["<< i << "]: "<< (unsigned int)packet[i] << std::endl;
		}
		return false;
	}

	for(i=0; i<4; i++)
	{
		endKey[i] = packet[i+SCMProtocol::SCMStartHeaderSize+len];
	}

	return setPayload(&packet[SCMProtocol::SCMStartHeaderSize],len);
}

bool SCMProtocol::SCMPacket::clearPayload()
{
	return setPayload(NULL,0);
}

bool SCMProtocol::SCMPacket::setPayload(unsigned char * payl, unsigned long long payl_len)
{
	unsigned long long i=0;
	try
	{
		len = payl_len;
		if(payload)
		{
			delete[] payload;
			payload=0;
		}

		if(!payl && !payl_len)
		{
			len=0;
			payload=0;
			return true;
		}

		payload = new unsigned char[payl_len];

		for(i=0;i<payl_len;i++)
		{
			payload[i] = payl[i];
		}

		return true;
	}
	catch(Exception &e)
	{
		std::cout << "SCMPacket Exception caught: " << e.description() << std::endl;
		return false;
	}
	return true;
}

bool SCMProtocol::SCMPacket::setHeader(SCMProtocol::PeerType _type, unsigned long long _id, SCMProtocol::Command cmd, unsigned char _channel)
{
	if(_channel<0 || _channel>SCM_MAX_CHANNELS)
	{
		return false;
	}

	this->type = static_cast<unsigned char>(_type);
	this->id = _id;
	this->command = static_cast<unsigned char>(cmd);
	this->channel = _channel;

	return true;
}

bool SCMProtocol::SCMPacket::setPeerSnitch()
{
	setPeerType(SCMProtocol::Client);
	return true;
}

bool SCMProtocol::SCMPacket::setPeerServer()
{
	setPeerType(SCMProtocol::Server);
	return true;
}

bool SCMProtocol::SCMPacket::setPeerType(SCMProtocol::PeerType _type)
{
	type = static_cast<unsigned char>(_type);
	return true;
}

bool SCMProtocol::SCMPacket::setPeerId(unsigned long long _id)
{
	id = _id;
	return true;
}

bool SCMProtocol::SCMPacket::setCommand(SCMProtocol::Command cmd)
{
	command = static_cast<unsigned char>(cmd);
	return true;
}

bool SCMProtocol::SCMPacket::setChannel(unsigned char _channel)
{
	channel = _channel;
	return true;
}


SCMProtocol::PeerType SCMProtocol::SCMPacket::getPeerType()
{
	return static_cast<SCMProtocol::PeerType>(type);
}

unsigned long long SCMProtocol::SCMPacket::getPeerId()
{
	return id;
}

SCMProtocol::Command SCMProtocol::SCMPacket::getCommand()
{
	return static_cast<SCMProtocol::Command>(command);
}
unsigned char SCMProtocol::SCMPacket::getChannel()
{
	return channel;
}

unsigned char * SCMProtocol::SCMPacket::getPayload(unsigned long long* out_len)
{
	unsigned long long i=0;
	unsigned char* paylRet;

	if(!payload || !len)
	{
		*out_len=0;
		return 0;
	}

	paylRet = new unsigned char[len];
	for(i=0;i<len;i++)
	{
		paylRet[i] = payload[i];
	}

	*out_len = len;
	return paylRet;
}

unsigned char * SCMProtocol::SCMPacket::getPacket(unsigned long long* out_len)
{
	unsigned long long int i=0;
	unsigned char* packetRet;
	unsigned long long initSize=0;
	unsigned long long endSize=len+SCMProtocol::SCMStartHeaderSize;

	if(!isHeaderValid())
	{
		*out_len=0;
		return 0;
	}

	packetRet = new unsigned char[len+SCMProtocol::SCMHeaderSize];

	initSize=0;
	endSize=SCMProtocol::SCMStartHeaderSize;

	for(i=initSize;i<endSize;i++)
	{
		if(i<4)
		{
			packetRet[i] = startKey[i];
		}
		else if(i<5)
		{
			packetRet[i] = type;
		}
		else if(i<13)
		{
			packetRet[i] = idBytes[i-5];
		}
		else if(i<14)
		{
			packetRet[i] = command;
		}
		else if(i<15)
		{
			packetRet[i] = channel;
		}
		else if(i<SCMProtocol::SCMStartHeaderSize)
		{
			packetRet[i] = lenBytes[i-15];
		}
		else
		{
			*out_len=0;
			return 0;
		}
	}

	if(len>0 && payload!=0)
	{
		initSize = SCMProtocol::SCMStartHeaderSize;
		endSize = len + SCMProtocol::SCMStartHeaderSize;
		for(i=initSize; i<endSize ; i++)
		{
			packetRet[i] = payload[i-initSize];
		}
	}

	initSize = len + SCMProtocol::SCMStartHeaderSize;
	endSize = len + SCMProtocol::SCMHeaderSize;
	for(i=initSize ; i<endSize ; i++)
	{
		packetRet[i] = endKey[i-initSize];
	}
	
	*out_len = SCMProtocol::SCMHeaderSize + len;
	return packetRet;
}

bool SCMProtocol::SCMPacket::isValid()
{
	return isHeaderValid();
}

bool SCMProtocol::SCMPacket::isHeaderValid()
{
	int i=0;

	for(i=0;i<4;i++)
	{
		if(startKey[i] != SCMProtocol::SCMStartKey[i] || 
			endKey[i] != SCMProtocol::SCMEndKey[i])
		{
			return false;
		}
	}

	if( (type<SCMProtocol::Server || type>SCMProtocol::Client) ||
		(id==0) ||
		(command<SCMProtocol::Register || command>SCMProtocol::HeartBeatAck))
	{
		return false;
	}

	return true;
}