#include "P2PConnection.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctype.h>
#include "Exception.h"

P2PPacket::P2PPacket()
{
	index=0;
	packet=0;
	size=0;

	is_begin=false;
	is_end=false;
}

P2PPacket::~P2PPacket()
{
	clearPacket();
}

P2PPacket::P2PPacket(unsigned char *_pkt, unsigned short _size, unsigned int _index)
{
	if(_pkt && _size>0)
	{
		size = (_size>MAX_P2P_PACKET_SIZE)?(MAX_P2P_PACKET_SIZE):(_size);
		packet = new unsigned char[size]();
		for(unsigned short i=0;i<size;i++)
		{
			packet[i] = _pkt[i];
		}
		index=_index;
	}
	else
	{
		index=_index;
		packet=0;
		size=0;
	}
	is_end=0;
	is_begin=0;
}

unsigned char P2PPacket::getByte(unsigned short position)
{
	if(!packet || position>=size)
	{
		return 0;
	}

	return packet[position];
}

bool P2PPacket::setByte(unsigned char byte, unsigned short position)
{
	if(!packet || position>=size)
	{
		return false;
	}

	packet[position] = byte;
}

unsigned short P2PPacket::getSize()
{
	if(!packet)
	{
		return 0;
	}
	return size;
}

void P2PPacket::setPacket(unsigned char *_pkt, unsigned short _size, unsigned int _index)
{
	if(_pkt && _size>0)
	{
		size = (_size>MAX_P2P_PACKET_SIZE)?(MAX_P2P_PACKET_SIZE):(_size);
	
		if(packet)
		{
			delete[] packet;
			packet=0;
		}

		packet = new unsigned char[size]();
		for(unsigned short i=0;i<size;i++)
		{
			packet[i] = _pkt[i];
		}
		index=_index;
	}
	else
	{
		clearPacket();
	}
}

unsigned char * P2PPacket::getPacket(unsigned short * out_size)
{
	*out_size = size;
	return packet;
}

void P2PPacket::setBegin()
{
	if(!is_end)
		is_begin=true;
}
void P2PPacket::setEnd()
{
	if(!is_begin)
		is_end=true;
}

void P2PPacket::clearPacket()
{
	if(packet)
	{
		delete[] packet;
	}
	packet=0;
	size=0;
	is_begin=false;
	is_end=false;
	index=0;
}

bool P2PPacket::isSet()
{
	if(packet && size>0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool P2PPacket::isBegin()
{
	return is_begin;
}

bool P2PPacket::isEnd()
{
	return is_end;
}

void P2PPacket::setIndex(unsigned int _index)
{
	index = _index;
}

unsigned int P2PPacket::getIndex()
{
	return index;
}

P2PBuffer::P2PBuffer(unsigned short max_size)
{
	packetCounter=0;
	packetIndex=0;
	oldestPosition=0;
	newestPosition=0;
	if(max_size<1)
	{
		maxSize=1;
	}
	else
	{
		maxSize = max_size;
	}
	firstPacket=true;
	clearBuffer();
}

P2PBuffer::~P2PBuffer()
{
	packetCounter=0;
	packetIndex=0;
	oldestPosition=0;
	newestPosition=0;
	maxSize=0;
	firstPacket=true;
	clearBuffer();
}

unsigned char * P2PBuffer::getPacket(unsigned short position, unsigned short *out_size)
{
	if(!isSet(position))
	{
		*out_size=0;
		return 0;
	}

	unsigned short _size=0;
	unsigned char *ret = buff[position].getPacket(&_size);

	*out_size=_size;

	return ret;
}

void P2PBuffer::setPacket(unsigned short position, unsigned char * _pkt, unsigned short _size)
{
	bool retransmission=false;
	if(!_pkt || _size==0)
	{
		return;
	}

	unsigned short ovflow = packetIndex+1;
	if(ovflow<=packetIndex) //IndexOverflow!
	{
		//std::cout << "buffer overflow" << std::endl;
		clearBuffer();
	}

	if(firstPacket)
	{
		firstPacket=false;
		oldestPosition=position;
	}

	if(buff[position].isSet())
	{
		retransmission=true;
		for(unsigned short i=0; i<((_size<200)?(_size):200); i++)
		{
			if(buff[position].getByte(i) != *(_pkt+i))
			{
				retransmission=false;
				break;
			}
		}

		if(!retransmission)
		{
			if(packetCounter>=maxSize)
			{
				buff[oldestPosition].clearPacket();
				packetCounter--;
				setOldestPacket();
			}
			
			buff[position].setPacket(_pkt, _size, packetIndex);
			packetIndex++;
			packetCounter++;
			newestPosition=position;
		}
	}
	else
	{
		if(packetCounter>=maxSize)
		{
			buff[oldestPosition].clearPacket();
			packetCounter--;
			setOldestPacket();
		}
		
		buff[position].setPacket(_pkt, _size, packetIndex);
		packetIndex++;
		packetCounter++;
		newestPosition=position;
	}
}

void P2PBuffer::setBegin(unsigned short position)
{
	buff[position].setBegin();
}

void P2PBuffer::setEnd(unsigned short position)
{
	buff[position].setEnd();
}

bool P2PBuffer::isBegin(unsigned short position)
{
	return buff[position].isBegin();
}

bool P2PBuffer::isEnd(unsigned short position)
{
	return buff[position].isEnd();
}

bool P2PBuffer::isSet(unsigned short position)
{
	return buff[position].isSet();
}

void P2PBuffer::clearPacket(unsigned short position)
{
	buff[position].clearPacket();
}

void P2PBuffer::clearBuffer()
{
	for(unsigned int i=0;i<65536;i++)
	{
		buff[i].clearPacket();
	}
	packetCounter=0;
	packetIndex=0;
	oldestPosition=0;
	newestPosition=0;
	firstPacket=true;
}

unsigned short P2PBuffer::getOldestPosition()
{
	return oldestPosition;
}

unsigned short P2PBuffer::getNewestPosition()
{
	return newestPosition;
}

void P2PBuffer::setOldestPacket()
{
	unsigned int min = 4294967295;
	unsigned short oldest = 0;
	unsigned int a;
	for(unsigned int i=0;i<65536;i++)
	{
		if(buff[i].isSet())
		{
			a = buff[i].getIndex();
			if(a<min)
			{
				min = a;
				oldest = i;
			}
		}
	}

	oldestPosition=oldest;
}

unsigned long int P2PBuffer::getRangeSize(unsigned short first_pos, unsigned short sec_pos)
{
	unsigned short firstPos, secPos, i;
	unsigned long int totalSize;

	if(first_pos>sec_pos)
	{
		firstPos=sec_pos;
		secPos=first_pos;
	}
	else
	{
		firstPos=first_pos;
		secPos=sec_pos;
	}

	totalSize=0;

	for(i=firstPos;i<=secPos;i++)
	{
		totalSize = totalSize + buff[i].getSize();
	}

	return totalSize;
}

P2PConnection::P2PConnection(unsigned long long _cameraId, unsigned char* _p2pId, unsigned int _p2pIdSize, unsigned char* _user, unsigned short _userSize, unsigned char* _password, unsigned short _passwordSize)
{
	union
	{
		unsigned short auxSize;
		unsigned char auxSizeBytes[2];
	};

	cameraId = _cameraId;
	for(unsigned int i=0;i<_p2pIdSize;i++)
	{
		p2pId[i] = _p2pId[i];
	}
	
	userSize = _userSize;
	user = new unsigned char[userSize]();
	for(unsigned short i=0; i<userSize; i++)
	{
		user[i] = _user[i];
	}

	passwordSize = _passwordSize;
	password = new unsigned char[passwordSize]();
	for(unsigned short i=0; i<passwordSize; i++)
	{
		password[i] = _password[i];
	}
	
	for(unsigned int i=0; i<17; i++)
	{
		msg1Remote[i+4] = p2pId[i];
		msg2[i+4] = p2pId[i];
		msg41[i+4] = p2pId[i];
		msg42[i+4] = p2pId[i];
		msg83[i+8] = p2pId[i];
		msg84[i+4] = p2pId[i];
		msg80[i+4] = p2pId[i];
	}

	ckUserSize=68+2*userSize+2*passwordSize;
	gtCamSize=ckUserSize;
	msghttpCheckUser = new unsigned char[ckUserSize]();
	msghttpGetCamera = new unsigned char[ckUserSize]();
	for(unsigned int i=0;i<ckUserSize;i++)
	{
		if(i<=45)
		{
			msghttpCheckUser[i] = msghttpCheckUser_base[i];
			msghttpGetCamera[i] = msghttpGetCamera_base[i];
		}
		else if(i>=46 && i<=(45+userSize))
		{
			msghttpCheckUser[i] = user[i-46];
			msghttpGetCamera[i] = user[i-46];
		}
		else if(i>=(46+userSize) && i<=(55+userSize))
		{
			msghttpCheckUser[i] = msghttpCheckUser_base[i-46-userSize+51];
			msghttpGetCamera[i] = msghttpGetCamera_base[i-46-userSize+51];
		}
		else if(i>=(56+userSize) && i<=(55+userSize+passwordSize))
		{
			msghttpCheckUser[i] = password[i-56-userSize];
			msghttpGetCamera[i] = password[i-56-userSize];
		}
		else if(i>=(56+userSize+passwordSize) && i<=(61+userSize+passwordSize))
		{
			msghttpCheckUser[i] = msghttpCheckUser_base[i-56-userSize-passwordSize+66];
			msghttpGetCamera[i] = msghttpGetCamera_base[i-56-userSize-passwordSize+66];
		}
		else if(i>=(62+userSize+passwordSize) && i<=(61+2*userSize+passwordSize))
		{
			msghttpCheckUser[i] = user[i-62-userSize-passwordSize];
			msghttpGetCamera[i] = user[i-62-userSize-passwordSize];
		}
		else if(i>=(62+2*userSize+passwordSize) && i<=(66+2*userSize+passwordSize))
		{
			msghttpCheckUser[i] = msghttpCheckUser_base[i-62-2*userSize-passwordSize+77];
			msghttpGetCamera[i] = msghttpGetCamera_base[i-62-2*userSize-passwordSize+77];
		}
		else if(i>=(67+2*userSize+passwordSize) && i<=(66+2*userSize+2*passwordSize))
		{
			msghttpCheckUser[i] = password[i-67-2*userSize-passwordSize];
			msghttpGetCamera[i] = password[i-67-2*userSize-passwordSize];
		}
		else if(i==(67+2*userSize+2*passwordSize))
		{
			msghttpCheckUser[i] = msghttpCheckUser_base[i-67-2*userSize-2*passwordSize+87];
			msghttpGetCamera[i] = msghttpGetCamera_base[i-67-2*userSize-2*passwordSize+87];
		}
		else
		{
			//error
		}
	}
	auxSize=ckUserSize-4;

	msghttpCheckUser[2] = auxSizeBytes[1];
	msghttpCheckUser[3] = auxSizeBytes[0];
	msghttpCheckUser[12] = msghttpCheckUser[3]-0x0C;
	msghttpGetCamera[2] = auxSizeBytes[1];
	msghttpGetCamera[3] = auxSizeBytes[0];
	msghttpGetCamera[12] = msghttpGetCamera[3]-0x0C;

	livStrSize=80+2*userSize+2*passwordSize;
	msghttpLiveStream = new unsigned char[livStrSize]();
	for(unsigned int i=0;i<livStrSize;i++)
	{
		if(i<=57)
		{
			msghttpLiveStream[i] = msghttpLiveStream_base[i];
		}
		else if(i>=58 && i<=(57+userSize))
		{
			msghttpLiveStream[i] = user[i-58];
		}
		else if(i>=(58+userSize) && i<=(67+userSize))
		{
			msghttpLiveStream[i] = msghttpLiveStream_base[i-58-userSize+63];
		}
		else if(i>=(68+userSize) && i<=(67+userSize+passwordSize))
		{
			msghttpLiveStream[i] = password[i-68-userSize];
		}
		else if(i>=(68+userSize+passwordSize) && i<=(73+userSize+passwordSize))
		{
			msghttpLiveStream[i] = msghttpLiveStream_base[i-68-userSize-passwordSize+78];
		}
		else if(i>=(74+userSize+passwordSize) && i<=(73+2*userSize+passwordSize))
		{
			msghttpLiveStream[i] = user[i-74-userSize-passwordSize];
		}
		else if(i>=(74+2*userSize+passwordSize) && i<=(78+2*userSize+passwordSize))
		{
			msghttpLiveStream[i] = msghttpLiveStream_base[i-74-2*userSize-passwordSize+89];
		}
		else if(i>=(79+2*userSize+passwordSize) && i<=(78+2*userSize+2*passwordSize))
		{
			msghttpLiveStream[i] = password[i-79-2*userSize-passwordSize];
		}
		else if(i==(79+2*userSize+2*passwordSize))
		{
			msghttpLiveStream[i] = msghttpLiveStream_base[i-79-2*userSize-2*passwordSize+99];
		}
		else
		{
			//error
		}
	}

	auxSize=livStrSize-4;
	msghttpLiveStream[2] = auxSizeBytes[1];
	msghttpLiveStream[3] = auxSizeBytes[0];
	msghttpLiveStream[12] = msghttpLiveStream[3]-0xC;

	capturingBuffer = new P2PBuffer(150);

	imageBeginIndex=0;
	imageEndIndex=0;
	imageBeginCurrentIndex=0;

	ret=Return_Nothing;
	connectionState=State_Waiting;
	cl_sock = new UDPSocket(0);

	connectionThreadId.id=0;
	connectionThreadId.created=false;
	pthread_mutex_init(&connectionThreadId.id_mutex,NULL);
	pthread_mutex_init(&connectionRunningMutex,NULL);
	pthread_mutex_init(&releaseMutex,NULL);
	connectionRunning=false;
	stopConnectionThread=false;
	releaseThread=false;

	pthread_mutex_init(&connectionStateMutex,NULL);
	pthread_mutex_init(&returnMutex,NULL);
	pthread_mutex_init(&capturingMutex,NULL);
}

P2PConnection::~P2PConnection()
{
	if(msghttpCheckUser)
	{
		delete[] msghttpCheckUser;
		msghttpCheckUser=0;
	}
	if(msghttpLiveStream)
	{
		delete[] msghttpLiveStream;
		msghttpLiveStream=0;
	}
	if(msghttpGetCamera)
	{
		delete[] msghttpGetCamera;
		msghttpGetCamera=0;
	}
	if(user)
	{
		delete[] user;
		user=0;
	}
	if(password)
	{
		delete[] password;
		password=0;
	}

	if(capturingBuffer)
	{
		delete capturingBuffer;
		capturingBuffer=0;
	}

	pthread_mutex_destroy(&connectionThreadId.id_mutex);
	pthread_mutex_destroy(&connectionRunningMutex);
	pthread_mutex_destroy(&connectionStateMutex);
	pthread_mutex_destroy(&returnMutex);
	pthread_mutex_destroy(&capturingMutex);
	pthread_mutex_destroy(&releaseMutex);

	delete cl_sock;
}

bool P2PConnection::start()
{
	return startConnection();
}

void P2PConnection::stop()
{
	stopConnection();
}

bool P2PConnection::isRunning()
{
	pthread_mutex_lock(&connectionRunningMutex);
	bool _ret = connectionRunning;
	pthread_mutex_unlock(&connectionRunningMutex);

	return _ret;
}

P2PReturn P2PConnection::getReturn()
{
	pthread_mutex_lock(&returnMutex);
	P2PReturn _ret = ret;
	pthread_mutex_unlock(&returnMutex);
	return _ret;
}

unsigned long long P2PConnection::getCameraId()
{
	return cameraId;
}

P2PConnectionState P2PConnection::getConnectionState()
{
	pthread_mutex_lock(&connectionStateMutex);
	P2PConnectionState _ret = connectionState;
	pthread_mutex_unlock(&connectionStateMutex);
	return _ret;
}

unsigned char * P2PConnection::getLastSnapshot(unsigned long int *out_size)
{
	unsigned short j=0;
	unsigned long i=0;
	unsigned short aux_size;
	unsigned long int _size=0;
	unsigned char * aux_snap=0;
	unsigned char * snap=0;
	unsigned long int snapIndex=0;
	bool imageCorrupted=false;
	//std::ofstream outputFile; //test

	pthread_mutex_lock(&connectionStateMutex);
	if(connectionState!=State_Streaming && connectionState!=State_AskingCamera)
	{
		pthread_mutex_unlock(&connectionStateMutex);
		*out_size=0;
		return 0;
	}
	pthread_mutex_unlock(&connectionStateMutex);

	pthread_mutex_lock(&capturingMutex);
	if((imageBeginIndex==0 && imageEndIndex==0) || (imageBeginIndex>=imageEndIndex))
	{
		pthread_mutex_unlock(&capturingMutex);
		*out_size=0;
		return 0;
	}

	_size = capturingBuffer->getRangeSize(imageBeginIndex,imageEndIndex);

	if(_size==0)
	{
		pthread_mutex_unlock(&capturingMutex);
		*out_size=0;
		return 0;
	}

	//std::cout << "Image Range: " << imageBeginIndex << " " << imageEndIndex << std::endl;

	//outputFile.open("snapshot-p2p.jpg",std::ios_base::app);
	snap = new unsigned char[_size]();
	snapIndex=0;
	
	for(j=imageBeginIndex;j<=imageEndIndex;j++)
	{
		if(!capturingBuffer->isSet(j))
		{
			imageCorrupted=true;
			break;
		}
		aux_snap = capturingBuffer->getPacket(j,&aux_size);

		for(i=0;i<aux_size;)
		{
			if((i+8) < aux_size)
			{
				if(aux_snap[i]==0xF1 && aux_snap[i+1]==0xD0 && aux_snap[i+4]==0xD1)
				{
					if((i+40) < aux_size)
					{
						if(aux_snap[i+8]==0x55 && aux_snap[i+9]==0xAA && aux_snap[i+10]==0x15 && aux_snap[i+11]==0xA8)
						{
							i = i+40;
							continue;
						}
						else
						{
							i = i+8;
							continue;
						}
					}
					else
					{
						i = i+8;
						continue;
					}
				}
				else
				{
					if(snapIndex < _size)
					{
						snap[snapIndex] = aux_snap[i];
						snapIndex++;
						//outputFile << aux_snap[i];
						i++;
						continue;
					}
					else
					{
						std::cout << "snapIndex error " << snapIndex << " " << _size << std::endl;
						i++;
						continue;
					}
				}
			}
			else
			{
				snap[snapIndex] = aux_snap[i];
				snapIndex++;
				//outputFile << aux_snap[i];
				i++;
				continue;
			}
		}
	}

	//outputFile.close();
	pthread_mutex_unlock(&capturingMutex);

	if(imageCorrupted)
	{
		//std::cout << "Image Corrupted" << std::endl;
		if(snap)
		{
			delete[] snap;
			snap = 0;
		}
		_size = 0;
	}
	else
	{
		_size=snapIndex+1;
	}

	//std::cout << "Snap size: "<< _size << std::endl;

	*out_size = _size;
	return snap;
}

void P2PConnection::performConnection()
{
	pthread_mutex_lock(&releaseMutex);
	releaseThread=true;
	pthread_mutex_unlock(&releaseMutex);
}

bool P2PConnection::startConnection()
{
	stopConnectionThread=false;

	pthread_mutex_lock(&connectionThreadId.id_mutex);
	bool _ret = (pthread_create(&connectionThreadId.id, NULL, runConnection, this) == 0);

	if(_ret)
	{
		connectionThreadId.created=true;
	}

	pthread_mutex_unlock(&connectionThreadId.id_mutex);
	return _ret;
}

void P2PConnection::stopConnection()
{
	stopConnectionThread=true;

	pthread_mutex_lock(&connectionThreadId.id_mutex);
	if(connectionThreadId.created)
	{
		pthread_join(connectionThreadId.id,NULL);
		connectionThreadId.created = false;
	}

	pthread_mutex_unlock(&connectionThreadId.id_mutex);
}

void* P2PConnection::runConnection(void* context)
{
	return ((P2PConnection*)context)->connectionThread();
}

void* P2PConnection::connectionThread()
{
	pthread_mutex_lock(&connectionRunningMutex);
	connectionRunning=true;
	pthread_mutex_unlock(&connectionRunningMutex);

	bool endpointConnected=false;

	std::vector<P2PEndpoint*>* endpoints = 0;
	int connectedIndex=-1;

	int cl_port = cl_sock->getLocalPort();

	char * cl_ip = cl_sock->getLocalIp();

	unsigned char ipBytes[4] = {0};
	size_t index = 0;

	bool streamingFinished=false;

	bool waitForRelease=false;

	if(waitForRelease)
	{
		while(1)
		{
			pthread_mutex_lock(&releaseMutex);
			if(releaseThread)
			{
				pthread_mutex_lock(&connectionStateMutex);
				connectionState = State_Connecting;
				pthread_mutex_unlock(&connectionStateMutex);

				pthread_mutex_unlock(&releaseMutex);
				break;
			}
			pthread_mutex_unlock(&releaseMutex);
			usleep(10000);
		}
	}
	else
	{
		pthread_mutex_lock(&connectionStateMutex);
		connectionState = State_Connecting;
		pthread_mutex_unlock(&connectionStateMutex);
	}


	try
	{
		while(1)
		{
			if(stopConnectionThread)
			{
				pthread_mutex_lock(&returnMutex);
				ret = Return_Disconnected;
				pthread_mutex_unlock(&returnMutex);
				//std::cout << "Disconnecting 1" << std::endl;
				break;
			}

			while (*cl_ip)
			{
				if (isdigit((unsigned char)*cl_ip))
				{
					ipBytes[index] *= 10;
					ipBytes[index] += *cl_ip - '0';
				}
				else
				{
					index++;
				}
				cl_ip++;
			}

			P2PEndpoint myEndpoint;

			myEndpoint.port = (unsigned short) cl_port;

			msg2[31] = (unsigned char)ipBytes[0];
			msg2[30] = (unsigned char)ipBytes[1];
			msg2[29] = (unsigned char)ipBytes[2];
			msg2[28] = (unsigned char)ipBytes[3];
			msg2[27] = myEndpoint.port_bytes[1];
			msg2[26] = myEndpoint.port_bytes[0];

			endpoints = discoverEndpoints();

			/*for(int i=0;i<endpoints->size();i++)
			{
				std::cout << endpoints->at(i)->ip << std::endl;
			}
			std::cout << "Discovered " << (unsigned int)p2pId[11] << std::endl;*/

			pthread_mutex_lock(&returnMutex);
			if(ret==Return_ServerTimedOut || ret==Return_ConnectionLoss)
			{
				pthread_mutex_unlock(&returnMutex);
				break;
			}
			pthread_mutex_unlock(&returnMutex);

			if(!endpoints)
			{
				std::cout << "error" << std::endl;
				break;
			}
			else if(endpoints->size()<=0)
			{
				pthread_mutex_lock(&returnMutex);
				ret = Return_ServerOffline;
				pthread_mutex_unlock(&returnMutex);
				std::cout << "No Endpoints ret " << ret << std::endl;
				break;
			}

			endpointConnected=false;
			connectedIndex=-1;
			if(openConnection(endpoints, true, &connectedIndex))
			{
				endpointConnected=true;
			}
			else
			{
				pthread_mutex_lock(&returnMutex);
				if(ret==Return_ServerTimedOut || ret==Return_ConnectionLoss)
				{
					pthread_mutex_unlock(&returnMutex);
					std::cout << "Server Timedout or ConnectionLoss" << std::endl;
					break;
				}
				else if (ret==Return_CameraTimedOut)
				{
					std::cout << "Camera Timedout" << std::endl;
					pthread_mutex_unlock(&returnMutex);	
					continue;
				}
				pthread_mutex_unlock(&returnMutex);
			}

			if(stopConnectionThread)
			{
				pthread_mutex_lock(&returnMutex);
				ret = Return_Disconnected;
				pthread_mutex_unlock(&returnMutex);
				//std::cout << "Disconnecting 2" << std::endl;
				break;
			}
			streamingFinished=false;

			if(endpointConnected && connectedIndex>=0)
			{
				streamingFinished=true;
				startStreaming(endpoints->at(connectedIndex));
			}

			{
				//Clearing Endpoint list
				P2PEndpoint * lastEndpoint;

				while(endpoints->size()>0)
				{
					lastEndpoint = endpoints->back();
					if(lastEndpoint)
					{
						delete lastEndpoint;
					}
					endpoints->pop_back();
				}
				endpoints->clear();
				if(endpoints)
				{
					delete endpoints;
					endpoints=0;
				}
			}
			pthread_mutex_lock(&returnMutex);
			if(ret==Return_WrongPassword)
			{
				pthread_mutex_unlock(&returnMutex);
				std::cout << "WrongPassword" << std::endl;
				break;
			}
			pthread_mutex_unlock(&returnMutex);
			
			if(stopConnectionThread)
			{
				pthread_mutex_lock(&returnMutex);
				ret = Return_Disconnected;
				pthread_mutex_unlock(&returnMutex);
				//std::cout << "Disconnecting 3" << std::endl;
				break;
			}

			if(streamingFinished)
			{
				std::cout << "streamingFinished" << std::endl;
				streamingFinished=false;
				continue;
			}

			endpoints = discoverRemoteEndpoints();
			
			if(ret==Return_ServerTimedOut || ret==Return_ConnectionLoss)
			{
				std::cout << "Return_ServerTimedOut || Return_ConnectionLoss" << std::endl;
				break;
			}

			if(!endpoints)
			{
				std::cout << "error 2" << std::endl;
				break;
			}
			else if(endpoints->size() <= 0)
			{
				pthread_mutex_lock(&returnMutex);
				ret = Return_CameraOffline;
				pthread_mutex_unlock(&returnMutex);
				std::cout << "No remote endpoints" << std::endl;
				break;
			}

			endpointConnected=false;
			connectedIndex=-1;
			if(openConnection(endpoints, false, &connectedIndex))
			{
				endpointConnected=true;
			}
			else
			{
				pthread_mutex_lock(&returnMutex);
				if(ret==Return_ServerTimedOut || ret==Return_ConnectionLoss)
				{
					pthread_mutex_unlock(&returnMutex);
					//std::cout << "Server Timedout or ConnectionLoss" << std::endl;
					break;
				}
				else if (ret==Return_CameraTimedOut)
				{
					//std::cout << "Camera Timedout remote" << std::endl;
					pthread_mutex_unlock(&returnMutex);	
					continue;
				}
				pthread_mutex_unlock(&returnMutex);
			}


			if(stopConnectionThread)
			{
				pthread_mutex_lock(&returnMutex);
				ret = Return_Disconnected;
				pthread_mutex_unlock(&returnMutex);
				//std::cout << "Disconnecting 4" << std::endl;
				break;
			}
			
			if(endpointConnected && connectedIndex>=0)
			{
				startStreaming(endpoints->at(connectedIndex));
			}

			{
				//Clearing Endpoint list
				P2PEndpoint * lastEndpoint;

				while(endpoints->size()>0)
				{
					lastEndpoint = endpoints->back();
					if(lastEndpoint)
					{
						delete lastEndpoint;
					}
					endpoints->pop_back();
				}
				endpoints->clear();
				if(endpoints)
				{
					delete endpoints;
					endpoints=0;
				}
			}
		}
	}
	catch(Exception &e)
	{
		std::cout << "Exception caught: " << e.description() << std::endl;
	}

	//DELETE ALLOCATED BYTES
	pthread_mutex_lock(&connectionRunningMutex);
	connectionRunning=false;
	pthread_mutex_unlock(&connectionRunningMutex);
	return 0;
}

std::vector<P2PPacketIndex*> * P2PConnection::splitPackets(unsigned char* data, int len)
{
	if(!data || !len)
	{
		return 0;
	}

	std::vector<P2PPacketIndex*> *ind_list = new std::vector<P2PPacketIndex*>;
	P2PPacketIndex * newIndex;
	P2PPacketIndex * lastIndex;
	int lastPacket=-1;
	for(int j=0;j<len;j++)
	{
		if(data[j]==0xF1)
		{
			if(lastPacket==-1)
			{
				lastPacket=0;
				newIndex = new P2PPacketIndex;
				newIndex->begin = j;
				ind_list->push_back(newIndex);
				//packetIndex[lastPacket][0]=j;
			}
			else
			{
				lastIndex = ind_list->back();
				lastIndex->end=j-1;
				//packetIndex[lastPacket][1]=j-1;
				lastPacket++;
				newIndex = new P2PPacketIndex;
				newIndex->begin = j;
				ind_list->push_back(newIndex);
				//packetIndex[lastPacket][0]=j;
			}
		}
	}

	lastIndex = ind_list->back();
	delete lastIndex;
	ind_list->pop_back();

	return ind_list;
}

std::vector<P2PEndpoint*> * P2PConnection::discoverEndpoints()
{
	long int recvSize;
	int recvErrno;

	unsigned char buffer[2000];
	int totalSize=0,noAnswerCount=0,recvCount=0, tries=0;
	bool noAnswer=false;

	char *senderHost;
	int senderPort;

	bool sendOk=true;

	//std::cout << buffer << std::endl;

	/*if(!cl_sock->bindRemoteAddr("54.214.22.83",32100)) //P2P Server
	{
		std::cout << "Couldn't bind to p2p server endpoint" << std::endl;
		return 0;
	}
	else
	{
		std::cout << "Bound to p2p server" << std::endl;
	}*/
	recvSize = cl_sock->receiveData((buffer+totalSize), (2000-totalSize), 100, &senderHost, &senderPort, &recvErrno);
	recvSize = cl_sock->receiveData((buffer+totalSize), (2000-totalSize), 100, &senderHost, &senderPort, &recvErrno);
	recvSize = cl_sock->receiveData((buffer+totalSize), (2000-totalSize), 1000, &senderHost, &senderPort, &recvErrno);

	recvSize=0;
	recvCount=0;
	tries=0;
	while(1)
	{
		if(tries>=3)
		{
			pthread_mutex_lock(&returnMutex);
			ret = Return_ServerTimedOut;
			pthread_mutex_unlock(&returnMutex);
			return 0;
		}

		sendOk=true;

		sendOk &= cl_sock->sendData(msg1,sizeof(msg1),"202.181.238.4",32100);
		sendOk &= cl_sock->sendData(msg1,sizeof(msg1),"212.227.88.30",32100);
		sendOk &= cl_sock->sendData(msg1,sizeof(msg1),"54.214.22.83",32100);

		sendOk &= cl_sock->sendData(msg2,sizeof(msg2),"202.181.238.4",32100);
		sendOk &= cl_sock->sendData(msg2,sizeof(msg2),"212.227.88.30",32100);
		sendOk &= cl_sock->sendData(msg2,sizeof(msg2),"54.214.22.83",32100);

		if(!sendOk)
		{
			pthread_mutex_lock(&returnMutex);
			ret = Return_ConnectionLoss;
			pthread_mutex_unlock(&returnMutex);
			return 0;
		}

		noAnswerCount=0;
		noAnswer=false;

		while(1)
		{
			recvSize=0;
			recvSize = cl_sock->receiveData((buffer+totalSize), (2000-totalSize), 1000, &senderHost, &senderPort, &recvErrno);

			if(recvSize==-1 && recvErrno!=EWOULDBLOCK && recvErrno!=EAGAIN)
			{
				pthread_mutex_lock(&returnMutex);
				ret = Return_ConnectionLoss;
				pthread_mutex_unlock(&returnMutex);
				return 0;
			}
			else if(recvSize==0)
			{
				pthread_mutex_lock(&returnMutex);
				ret = Return_ConnectionLoss;
				pthread_mutex_unlock(&returnMutex);
				return 0;
			}
			else if(recvSize>0)
			{
				recvCount++;
				noAnswerCount=0;
				noAnswer=false;
				totalSize = totalSize + recvSize;
				if(recvCount>=6)
				{
					noAnswer=false;
					break; //Done
				}

				if(totalSize>=2000)
				{
					totalSize=2000; // Too Long
					break;
				}
			}
			else
			{
				if(noAnswerCount>=3)
				{
					noAnswer=true;
					break;
				}
				noAnswerCount++;
				//No data received
			}
		}
		if(!noAnswer)
		{
			break;
		}
		tries++;
	}

	/*std::cout << std::hex;
	for(int i=0;i<totalSize;i++)
	{
		std::cout << (unsigned int)buffer[i] << " ";
	}
	std::cout << std::dec << "discover " << senderHost << " " << (unsigned int)p2pId[11] << std::endl;*/


	std::vector<P2PPacketIndex*> * indexes = splitPackets(buffer, totalSize);
	std::vector<P2PEndpoint*> * endpoints = new std::vector<P2PEndpoint*>;
	P2PEndpoint *newEndpoint;
	unsigned char ip_bytes[4];

	std::stringstream ipstream;
	bool repeatedEnpoint=false;

	for(unsigned int i=0;i<indexes->size();i++)
	{
		if(buffer[indexes->at(i)->begin+1]==0x40) //Then this packet contains the IP and PORT
		{
			ip_bytes[0] = buffer[indexes->at(i)->begin+11];
			ip_bytes[1] = buffer[indexes->at(i)->begin+10];
			ip_bytes[2] = buffer[indexes->at(i)->begin+9];
			ip_bytes[3] = buffer[indexes->at(i)->begin+8];

			ipstream << (unsigned int)ip_bytes[0] << "." << (unsigned int)ip_bytes[1] << "." << (unsigned int)ip_bytes[2] << "." << (unsigned int)ip_bytes[3];
			newEndpoint = new P2PEndpoint;
			newEndpoint->ip = ipstream.str();
			ipstream.str("");
			ipstream.clear();

			newEndpoint->port_bytes[1] = buffer[indexes->at(i)->begin+7];
			newEndpoint->port_bytes[0] = buffer[indexes->at(i)->begin+6];
			newEndpoint->directConnection=true;

			repeatedEnpoint=false;
			for(unsigned int j=0;i<endpoints->size();j++)
			{
				if(endpoints->at(j)->ip==newEndpoint->ip)
				{
					repeatedEnpoint=true;
					break;
				}
			}

			if(!repeatedEnpoint && endpoints->size()<2)
			{
				//std::cout << "Discovered Endpoint: " << newEndpoint->ip << ":" << newEndpoint->port << " " << p2pId[11] << std::endl;
				endpoints->push_back(newEndpoint);
			}
		}
	}

	/****Clearing Index list****/
	P2PPacketIndex * lastIndex;

	while(indexes->size()>0)
	{
		lastIndex = indexes->back();
		if(lastIndex)
		{
			delete lastIndex;
		}
		indexes->pop_back();
	}
	indexes->clear();
	if(indexes)
	{
		delete indexes;
	}
	/******************/

	return endpoints;
}

std::vector<P2PEndpoint*> * P2PConnection::discoverRemoteEndpoints()
{
	long int recvSize;
	int recvErrno;

	unsigned char buffer[2000];
	unsigned int totalSize=0,noAnswerCount=0,recvCount=0,tries=0;
	bool noAnswer=false;

	char *senderHost;
	int senderPort;
	bool sendOk=true;
	/*if(!cl_sock->bindRemoteAddr("54.214.22.83",32100)) //P2P Server
	{
		std::cout << "Couldn't bind to p2p server endpoint" << std::endl;
		return 0;
	}
	else
	{
		std::cout << "Bound to p2p server" << std::endl;
	}*/

	recvCount=0;
	tries=0;
	while(1)
	{
		if(tries>=3)
		{
			pthread_mutex_lock(&returnMutex);
			ret = Return_ServerTimedOut;
			pthread_mutex_unlock(&returnMutex);
			return 0;
		}

		sendOk=true;

		sendOk &= cl_sock->sendData(msg1Remote,sizeof(msg1Remote),"202.181.238.4",32100);
		sendOk &= cl_sock->sendData(msg1Remote,sizeof(msg1Remote),"212.227.88.30",32100);
		sendOk &= cl_sock->sendData(msg1Remote,sizeof(msg1Remote),"54.214.22.83",32100);

		if(!sendOk)
		{
			pthread_mutex_lock(&returnMutex);
			ret = Return_ConnectionLoss;
			pthread_mutex_unlock(&returnMutex);
			return 0;
		}

		noAnswerCount=0;
		noAnswer=false;
		while(1)
		{
			recvSize=0;
			recvSize = cl_sock->receiveData((buffer+totalSize), (2000-totalSize), 500, &senderHost, &senderPort, &recvErrno);

			if(recvSize==-1 && recvErrno!=EWOULDBLOCK && recvErrno!=EAGAIN)
			{
				pthread_mutex_lock(&returnMutex);
				ret = Return_ConnectionLoss;
				pthread_mutex_unlock(&returnMutex);
				return 0;
			}
			else if(recvSize==0)
			{
				pthread_mutex_lock(&returnMutex);
				ret = Return_ConnectionLoss;
				pthread_mutex_unlock(&returnMutex);
				return 0;
			}
			else if(recvSize>0)
			{
				recvCount++;
				noAnswerCount=0;
				noAnswer=false;
				totalSize = totalSize + recvSize;
				if(recvCount>=2)
				{
					noAnswer=false;
					break; //Done
				}

				if(totalSize>=2000)
				{
					totalSize=2000; // Too Long
					break;
				}
			}
			else
			{
				if(noAnswerCount>=3)
				{
					noAnswer=true;
					break;
				}
				noAnswerCount++;
				//No data received
			}
		}
		if(!noAnswer)
		{
			break;
		}
		tries++;
	}

	/*std::cout << std::hex;
	for(int i=0;i<totalSize;i++)
	{
		std::cout << (unsigned int)buffer[i] << " ";
	}
	std::cout << std::dec << "discoverRemote " << senderHost << " " << (unsigned int)p2pId[11] << std::endl;*/


	std::vector<P2PEndpoint*> * endpoints = new std::vector<P2PEndpoint*>;
	P2PEndpoint *newEndpoint;
	unsigned char ip_bytes[4];

	std::stringstream ipstream;

	for(unsigned int i=0;i<totalSize;i++)
	{
		if(i<(totalSize-1) && buffer[i]==0xf1 && buffer[i+1]==0x69) //Then this packet contains the IP and PORT
		{
			unsigned char ipNumber=buffer[i+4];
			unsigned int begin;

			for(unsigned int j=0;j<ipNumber; j++)
			{
				begin = 16*j + 9 + i;
				if(begin+6>totalSize)
				{
					break;
				}
				ip_bytes[0] = buffer[begin+6];
				ip_bytes[1] = buffer[begin+5];
				ip_bytes[2] = buffer[begin+4];
				ip_bytes[3] = buffer[begin+3];

				ipstream << (unsigned int)ip_bytes[0] << "." << (unsigned int)ip_bytes[1] << "." << (unsigned int)ip_bytes[2] << "." << (unsigned int)ip_bytes[3];
				newEndpoint = new P2PEndpoint;
				newEndpoint->ip = ipstream.str();
				ipstream.str("");
				ipstream.clear();

				newEndpoint->port_bytes[1] = buffer[begin+2];
				newEndpoint->port_bytes[0] = buffer[begin+1];
				newEndpoint->directConnection=false;

				endpoints->push_back(newEndpoint);
			}
			break;
		}
	}

	return endpoints;
}



bool P2PConnection::openConnection(std::vector<P2PEndpoint*> * endpoints, bool directConnection, int *out_connectedEndpointIndex)
{
	long int recvSize;
	int recvErrno;

	unsigned char buffer[1000];
	int noAnswerCount=0,tries=0;
	bool noAnswer=false, cameraOk=false;
	bool send70=true;
	char *senderHost;
	int senderPort;
	bool anotherEP=false;

	while(1)
	{
		tries=0;
		while(tries<5)
		{
			for(unsigned int j=0;j<endpoints->size();j++)
			{
				if(directConnection)
				{
					cl_sock->sendData(msg41, sizeof(msg41),endpoints->at(j)->ip,endpoints->at(j)->port);
				}
				else
				{
					if(send70)
					{
						cl_sock->sendData(msg70, sizeof(msg70),endpoints->at(j)->ip,endpoints->at(j)->port);
					}
				}
			}

			noAnswerCount=0;
			noAnswer=false;
			cameraOk=false;
			while(1)
			{
				recvSize=0;
				recvSize = cl_sock->receiveData((buffer), (1000), 500, &senderHost, &senderPort, &recvErrno);

				if(recvSize==-1 && recvErrno!=EWOULDBLOCK && recvErrno!=EAGAIN)
				{
					pthread_mutex_lock(&returnMutex);
					ret = Return_ConnectionLoss;
					pthread_mutex_unlock(&returnMutex);
					break;
				}
				else if(recvSize==0)
				{
					pthread_mutex_lock(&returnMutex);
					ret = Return_ConnectionLoss;
					pthread_mutex_unlock(&returnMutex);
					break;
				}
				else if(recvSize>0)
				{
					std::string host(senderHost);
					anotherEP=true;
					for(unsigned int j=0;j<endpoints->size();j++)
					{
						//std::cout << "Enpoint[" << j << "]: " << endpoints->at(j)->ip << ":" << endpoints->at(j)->port << std::endl;
						if(endpoints->at(j)->ip==host && endpoints->at(j)->port==senderPort)
						{
							anotherEP=false;
							break;
						}
					}

					if(anotherEP)
					{
						//std::cout << "AnotherEp: " << host << ":" << senderPort << std::endl;
						continue;
					}

					noAnswerCount=0;
					/*std::cout << std::hex;
					for(int i=0;i<recvSize;i++)
					{
						std::cout << (unsigned int)buffer[i] << " ";
					}
					std::cout << std::dec << "openConnection " << senderHost << " " << (unsigned int)p2pId[11] << std::endl;
					*/

					for(int i=0;i<recvSize;i++)
					{
						if(i<(recvSize-1) && buffer[i]==0xf1 && buffer[i+1]==0xf0)
						{
							cl_sock->sendData(msgf0_final, sizeof(msgf0_final),host,senderPort);
							
							pthread_mutex_lock(&returnMutex);
							ret = Return_CameraTimedOut;
							pthread_mutex_unlock(&returnMutex);
							return false;
						}
						if(directConnection)
						{
							if(i<(recvSize-1) && buffer[i]==0xf1 && buffer[i+1]==0x41)
							{
								cl_sock->sendData(msg42, sizeof(msg42),host,senderPort);
							}
							if(i<(recvSize-1) && buffer[i]==0xf1 && buffer[i+1]==0x42)
							{
								cameraOk=true;
								cl_sock->sendData(msge0, sizeof(msge0),host,senderPort);
								*out_connectedEndpointIndex=-1;
								for(unsigned int j=0;j<endpoints->size();j++)
								{
									if(host==endpoints->at(j)->ip)
									{
										*out_connectedEndpointIndex = j;
										break;
									}
								}
								break;
							}
						}
						else
						{
							if(i<(recvSize-1) && buffer[i]==0xf1 && buffer[i+1]==0x84)
							{
								cameraOk=true;
								cl_sock->sendData(msg84, sizeof(msg84),host,senderPort);
								cl_sock->sendData(msge0, sizeof(msge0),host,senderPort);
								//std::cout << "Received 84" << std::endl;
								*out_connectedEndpointIndex=-1;
								for(unsigned int j=0;j<endpoints->size();j++)
								{
									if(host==endpoints->at(j)->ip)
									{
										*out_connectedEndpointIndex = j;
										break;
									}
								}
								break;
							}
							if(i<(recvSize-1) && buffer[i]==0xf1 && buffer[i+1]==0x83)
							{
								cameraOk=true;
								cl_sock->sendData(msge0, sizeof(msge0),host,senderPort);
								//std::cout << "Received 83" << std::endl;
								*out_connectedEndpointIndex=-1;
								for(unsigned int j=0;j<endpoints->size();j++)
								{
									if(host==endpoints->at(j)->ip)
									{
										*out_connectedEndpointIndex = j;
										break;
									}
								}
								break;
							}
							if(i<(recvSize-1) && buffer[i]==0xf1 && buffer[i+1]==0x71)
							{
								//send70=false;
								cl_sock->sendData(msg72, sizeof(msg72),host,senderPort);
								//std::cout << "Received 71" << std::endl;
								break;
							}
							if(i<(recvSize-1) && buffer[i]==0xf1 && buffer[i+1]==0x73)
							{
								P2PEndpoint _incomingEndpoint;
								_incomingEndpoint.port = (unsigned short)senderPort;
								char ipBytes_[4] = {0};
								char * hostPointer = senderHost;
								size_t index = 0;
								while (*hostPointer)
								{
									if (isdigit((unsigned char)*hostPointer))
									{
										ipBytes_[index] *= 10;
										ipBytes_[index] += *hostPointer - '0';
									}
									else
									{
										index++;
									}
									hostPointer++;
								}


								msg80[i+26] = _incomingEndpoint.port_bytes[0];
								msg80[i+27] = _incomingEndpoint.port_bytes[1];
								msg80[i+28] = (unsigned char)ipBytes_[3];
								msg80[i+29] = (unsigned char)ipBytes_[2];
								msg80[i+30] = (unsigned char)ipBytes_[1];
								msg80[i+31] = (unsigned char)ipBytes_[0];

								msg80[40] = buffer[i+4];
								msg80[41] = buffer[i+5];
								msg80[42] = buffer[i+6];
								msg80[43] = buffer[i+7];
								cl_sock->sendData(msg80,sizeof(msg80),"202.181.238.4",32100);
								cl_sock->sendData(msg80,sizeof(msg80),"212.227.88.30",32100);
								cl_sock->sendData(msg80,sizeof(msg80),"54.214.22.83",32100);
								//send70 = false;
								//std::cout << "Received 73" << std::endl;
								break;
							}
							if(i<(recvSize-1) && buffer[i]==0xf1 && buffer[i+1]==0x82)
							{
								std::stringstream ipstream_;
								P2PEndpoint endpoint_;
								unsigned char ipBytes_[4] = {0};

								ipBytes_[0] = buffer[i+11];
								ipBytes_[1] = buffer[i+10];
								ipBytes_[2] = buffer[i+9];
								ipBytes_[3] = buffer[i+8];
								ipstream_ << (unsigned int)ipBytes_[0] << "." << (unsigned int)ipBytes_[1] << "." << (unsigned int)ipBytes_[2] << "." << (unsigned int)ipBytes_[3];
								
								endpoint_.port_bytes[1] = buffer[i+7];
								endpoint_.port_bytes[0] = buffer[i+6];

								endpoint_.ip = ipstream_.str();

								msg83[4] = buffer[i+20];
								msg83[5] = buffer[i+21];
								msg83[6] = buffer[i+22];
								msg83[7] = buffer[i+23];
								cl_sock->sendData(msg83, sizeof(msg83),endpoint_.ip,endpoint_.port);
								//send70 = false;
								//std::cout << "Received 82" << std::endl;
								break;
							}
						}
					}
					if(cameraOk)
					{
						break;
					}
				}
				else
				{
					if(noAnswerCount>=6)
					{
						noAnswer=true;
						break;
					}
					noAnswerCount++;
					//No data received
				}
			}
			if(!noAnswer || cameraOk)
			{
				break;
			}
			tries++;
		}

		if(noAnswer)
		{
			pthread_mutex_lock(&returnMutex);
			ret = Return_ServerTimedOut;
			pthread_mutex_unlock(&returnMutex);
			return false;
		}

		if(cameraOk)
		{
			break;
		}
	}

	if(cameraOk)
	{
		return true;
	}
	else
	{
		pthread_mutex_lock(&returnMutex);
		ret = Return_CameraOffline;
		pthread_mutex_unlock(&returnMutex);
		return false;
	}

	return true;
}


void P2PConnection::startStreaming(P2PEndpoint* endpoint)
{
	long int recvSize;
	int recvErrno;
	unsigned int streamingCounter=0;
	unsigned int streamingE0Counter=0;

	unsigned char buffer[5000];
	bool waiting=false;

	unsigned char msgd1_ack[10] = {0xf1,0xd1,0x00,0x06,0xd1,0x01,0x00,0x01,0x00,0x00};
	std::ofstream outputFile;
	char *senderHost;
	int senderPort;
	unsigned int streamingE0=0;
	unsigned int noAnswer=0;

	if(endpoint->directConnection)
	{
		pthread_mutex_lock(&connectionStateMutex);
		connectionState=State_OpenningChannel;
		pthread_mutex_unlock(&connectionStateMutex);
	}
	else
	{
		pthread_mutex_lock(&connectionStateMutex);
		connectionState=State_OpenningChannel;
		pthread_mutex_unlock(&connectionStateMutex);
	}

	while(1)
	{
		if(stopConnectionThread)
		{
			pthread_mutex_lock(&returnMutex);
			ret = Return_Disconnected;
			pthread_mutex_unlock(&returnMutex);
			//std::cout << "Disconnecting 0" << std::endl;
			break;
		}

		if(!waiting)
		{
			pthread_mutex_lock(&connectionStateMutex);
			if(connectionState==State_AskingCamera)
			{
				pthread_mutex_unlock(&connectionStateMutex);
				cl_sock->sendData(msge1, sizeof(msge1), endpoint->ip, (unsigned int)(endpoint->port));
				cl_sock->sendData(msghttpCheckUser, ckUserSize, endpoint->ip, (unsigned int)(endpoint->port));
				cl_sock->sendData(msghttpGetCamera, gtCamSize, endpoint->ip, (unsigned int)(endpoint->port));
				cl_sock->sendData(msghttpLiveStream, livStrSize, endpoint->ip, (unsigned int)(endpoint->port));
				waiting=true;
				//std::cout << "Asked" << std::endl;
			}
			else
			{
				pthread_mutex_unlock(&connectionStateMutex);
			}
		}

		recvSize=0;
		recvSize = cl_sock->receiveData((buffer), (5000), 500, &senderHost, &senderPort, &recvErrno);

		if(recvSize==-1 && recvErrno!=EWOULDBLOCK && recvErrno!=EAGAIN)
		{
			pthread_mutex_lock(&returnMutex);
			ret = Return_ConnectionLoss;
			pthread_mutex_unlock(&returnMutex);
			break;
		}
		else if(recvSize==0)
		{
			//Connection Loss
			pthread_mutex_lock(&returnMutex);
			ret = Return_ConnectionLoss;
			pthread_mutex_unlock(&returnMutex);
			break;
		}
		else if(recvSize>0)
		{
			noAnswer=0;

			/*std::cout << std::hex;
			for(int i=0;i<recvSize;i++)
			{
				std::cout << (unsigned int)buffer[i] << " ";
			}
			std::cout << std::dec << "streaming" << std::endl;*/

			for(int i=0;i<recvSize;i++)
			{
				pthread_mutex_lock(&connectionStateMutex);
				if(connectionState==State_OpenningChannel)
				{
					pthread_mutex_unlock(&connectionStateMutex);
					if(endpoint->directConnection)
					{
						if(i<(recvSize-1) && buffer[i]==0xf1 && buffer[i+1]==0x41)
						{
							std::string host(senderHost);
							if(host==endpoint->ip)
							{
								cl_sock->sendData(msg42, sizeof(msg42), endpoint->ip, (unsigned int)(endpoint->port));
								//std::cout << "Received 41" << std::endl;
							}
							else
							{
								//std::cout << "Received 41 from other endpoint" << std::endl;
							}

							break;
						}
						else if(i<(recvSize-1) && buffer[i]==0xf1 && buffer[i+1]==0x42)
						{
							std::string host(senderHost);
							if(host==endpoint->ip)
							{
								cl_sock->sendData(msge0, sizeof(msge0), endpoint->ip, (unsigned int)(endpoint->port));
								waiting=false;
								pthread_mutex_lock(&connectionStateMutex);
								connectionState=State_AskingCamera;
								pthread_mutex_unlock(&connectionStateMutex);
								//std::cout << "Received 42" << std::endl;
							}
							else
							{
								//std::cout << "Received 42 from other endpoint" << std::endl;
							}
							break;
						}
					}
					else
					{
						if(i<(recvSize-1) && buffer[i]==0xf1 && buffer[i+1]==0x84)
						{
							std::string host(senderHost);
							if(host==endpoint->ip)
							{
								cl_sock->sendData(msg84, sizeof(msg84),host,senderPort);
								cl_sock->sendData(msge0, sizeof(msge0),host,senderPort);
								//std::cout << "Received 84" << std::endl;
							}
							else
							{
								//std::cout << "Received 84 from other endpoint" << std::endl;
							}
							break;
						}
						else if(i<(recvSize-1) && buffer[i]==0xf1 && buffer[i+1]==0x83)
						{
							std::string host(senderHost);
							if(host==endpoint->ip)
							{
								cl_sock->sendData(msge0, sizeof(msge0),host,senderPort);
								//std::cout << "Received 83" << std::endl;
			
								pthread_mutex_lock(&connectionStateMutex);
								connectionState = State_AskingCamera;
								pthread_mutex_unlock(&connectionStateMutex);
							}
							else
							{
								//std::cout << "Received 83 from other endpoint" << std::endl;
							}
							break;
						}
					}
				}
				else if(connectionState==State_AskingCamera)
				{
					pthread_mutex_unlock(&connectionStateMutex);
					if(i<(recvSize-1) && buffer[i]==0xf1 && buffer[i+1]==0xd0)
					{
						std::string host(senderHost);
						if(host==endpoint->ip)
						{
							msgd1_ack[8] = buffer[i+6];
							msgd1_ack[9] = buffer[i+7];

							cl_sock->sendData(msgd1_ack, sizeof(msgd1_ack),endpoint->ip,(unsigned int)(endpoint->port));

							{
								std::string ans((char *)buffer,recvSize);
								if(ans.find("result=-1")!=std::string::npos)
								{
									//std::cout << "Wrong Password" << std::endl;
									pthread_mutex_lock(&returnMutex);
									ret = Return_WrongPassword;
									pthread_mutex_unlock(&returnMutex);
									return;
								}
								else if(ans.find("result=0")!=std::string::npos)
								{
									//std::cout << "Right Password" << std::endl;
								}
								else if(ans.find("JFIF")!=std::string::npos)
								{
									union
									{
										unsigned char idxBytes[2];
										unsigned short idx;
									};

									idxBytes[0] = buffer[i+6];
									idxBytes[1] = buffer[i+7];

									imageBeginCurrentIndex=0;
									pthread_mutex_lock(&capturingMutex);
									capturingBuffer->setPacket(idx, buffer, recvSize);
									capturingBuffer->setBegin(idx);
									pthread_mutex_unlock(&capturingMutex);

									/*outputFile.open("streaming_log",std::ios_base::app);
									for(j=i+0; j<recvSize; j++)
									{
										outputFile << buffer[j];
									}
									outputFile.close();*/
									std::cout << "Starting Streaming" << std::endl;
									pthread_mutex_lock(&connectionStateMutex);
									connectionState = State_Streaming;
									pthread_mutex_unlock(&connectionStateMutex);
								}
							}
							//std::cout << "Received d0" << std::endl;
						}
						else
						{
							//std::cout << "Received d0 from other endpoint" << std::endl;
						}
						break;
					}
					else if(i<(recvSize-1) && buffer[i]==0xf1 && buffer[i+1]==0xd1)
					{
						std::string host(senderHost);
						if(host==endpoint->ip)
						{
							//cl_sock->sendData(msge1, sizeof(msge1));
							cl_sock->sendData(msge0, sizeof(msge0), endpoint->ip,(unsigned int)(endpoint->port));
							//std::cout << "Received d1" << std::endl;
						}
						else
						{
							//std::cout << "Received d1 from other endpoint" << std::endl;
						}
						break;
					}
					else if(i<(recvSize-1) && buffer[i]==0xf1 && buffer[i+1]==0xe0)
					{
						std::string host(senderHost);
						if(host==endpoint->ip)
						{
							cl_sock->sendData(msge1, sizeof(msge1),endpoint->ip,(unsigned int)(endpoint->port));
							streamingCounter++;
							if(streamingCounter>=4)
							{
								streamingCounter=0;
								waiting=false;
							}
							//std::cout << "Received e0" << std::endl;
						}
						else
						{
							//std::cout << "Received e0 from other endpoint" << std::endl;
						}

						break;
						//cl_sock->sendData(msge1, sizeof(msge1));
						//connectionState=State_Streaming;
					}
					else if(i<(recvSize-1) && buffer[i]==0xf1 && buffer[i+1]==0xe1)
					{
						std::string host(senderHost);
						if(host==endpoint->ip)
						{
							//std::cout << "Received e1" << std::endl;
						}
						else
						{
							//std::cout << "Received e1 from other endpoint" << std::endl;

						}

						break;
					}
				}
				else if(connectionState==State_Streaming)
				{
					pthread_mutex_unlock(&connectionStateMutex);
					if(i<(recvSize-1) && buffer[i]==0xf1 && buffer[i+1]==0xd0)
					{
						std::string host(senderHost);
						if(host==endpoint->ip)
						{
							streamingE0=0;
							msgd1_ack[8] = buffer[i+6];
							msgd1_ack[9] = buffer[i+7];
							cl_sock->sendData(msgd1_ack, sizeof(msgd1_ack),endpoint->ip,(unsigned int)(endpoint->port));

							if((i+0) < recvSize)
							{
								{
									std::string ans((char *)buffer,recvSize);

									unsigned char header[] = {0xFF, 0xD8};
									unsigned char footer[] = {0xFF, 0xD9};
									std::string needleH(header, header + 2);
									std::string needleF(footer, footer + 2);

									union
									{
										unsigned char idxBytes[2];
										unsigned short idx;
									};

									idxBytes[0] = buffer[i+7];
									idxBytes[1] = buffer[i+6];

									pthread_mutex_lock(&capturingMutex);
									capturingBuffer->setPacket(idx, buffer, recvSize);

									if(ans.find(needleH)!=std::string::npos) // Image Beginning
									{
										capturingBuffer->setBegin(idx);
										imageBeginCurrentIndex = idx;
									}

									//Connection Log
									/*outputFile.open("streaming_log",std::ios_base::app);
									outputFile << idx << " " << recvSize << std::endl;
									outputFile.close();*/

									if(ans.find(needleF)!=std::string::npos && idx>imageBeginCurrentIndex) //Image Finishing
									{
										imageBeginIndex=imageBeginCurrentIndex;
										imageEndIndex=idx;
										capturingBuffer->setEnd(idx);
										//std::cout << "Image Finished " << imageBeginIndex << " " << imageEndIndex << std::endl;
									}

									pthread_mutex_unlock(&capturingMutex);
									
								}

								/*{
									unsigned int j=0;
									outputFile.open("streaming_log",std::ios_base::app);
									for(j=i+0; j<recvSize; j++)
									{
										outputFile << buffer[j];
									}
									outputFile.close();

									streamingCounter++;
									if(streamingCounter==45)
									{
										streamingCounter=0;
										std::cout << "streaming received " << std::endl;
									}
								}*/
							}
						}
						else
						{
							//std::cout << "Streaming received from other endpoint" << std::endl;
						}
						break;
					}
					else if(i<(recvSize-1) && buffer[i]==0xf1 && buffer[i+1]==0xe0)
					{
						std::string host(senderHost);
						if(host==endpoint->ip)
						{
							cl_sock->sendData(msge1, sizeof(msge1),endpoint->ip,(unsigned int)(endpoint->port));
							streamingE0++;
							if(streamingE0>=100)
							{
								cl_sock->sendData(msge0, sizeof(msge0),endpoint->ip,(unsigned int)(endpoint->port));
								cl_sock->sendData(msge0, sizeof(msge0),endpoint->ip,(unsigned int)(endpoint->port));
								cl_sock->sendData(msge0, sizeof(msge0),endpoint->ip,(unsigned int)(endpoint->port));
								cl_sock->sendData(msge0, sizeof(msge0),endpoint->ip,(unsigned int)(endpoint->port));
								cl_sock->sendData(msgf0_final, sizeof(msgf0_final),endpoint->ip,(unsigned int)(endpoint->port));
								//std::cout << "Finishing communication" << std::endl;
								return;
							}
							streamingE0Counter++;
							if(streamingE0Counter==45)
							{
								streamingE0Counter=0;
								//std::cout << "Received 45 e0" << std::endl;
							}
						}
						else
						{
							//std::cout << "Received e0 from other endpoint" << std::endl;
						}
						break;
					}
					else if(i<(recvSize-1) && buffer[i]==0xf1 && buffer[i+1]==0xf0)
					{
						std::string host(senderHost);
						if(host==endpoint->ip)
						{
							cl_sock->sendData(msgf0_final, sizeof(msgf0_final),endpoint->ip,(unsigned int)(endpoint->port));
							return;
						}
						else
						{
							//std::cout << "Received f0 from other endpoint" << std::endl;
						}
					}
				}
				else if(connectionState==State_Disconnected)
				{
					pthread_mutex_unlock(&connectionStateMutex);
					return;
				}
				else
				{
					pthread_mutex_unlock(&connectionStateMutex);
				}

				//printf("%x ", buffer[i]);
			}
		}
		else
		{
			noAnswer++;
			if(noAnswer>=10)
			{
				pthread_mutex_lock(&returnMutex);
				ret = Return_CameraTimedOut;
				pthread_mutex_unlock(&returnMutex);

				pthread_mutex_lock(&connectionStateMutex);
				connectionState = State_Disconnected;	
				pthread_mutex_unlock(&connectionStateMutex);
		
				break;
			}
		}
	}
}