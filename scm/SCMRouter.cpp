#include "SCMRouter.h"
#include "Exception.h"
#include <iostream>
#include "errno.h"
#include <fstream>
#include <sched.h>


SCMHalfChannel::SCMHalfChannel(DataSocket * _in, DataSocket * _out, unsigned int buffer_size, unsigned int size_overhead, unsigned char _chnNumber, SCMProtocol::PeerType _mpeerType, unsigned long long _mypeerId) : 
HalfDuplexChannel::HalfDuplexChannel(_in, _out, buffer_size, size_overhead)
{
	if(_chnNumber>=0 && _chnNumber<=SCM_MAX_CHANNELS && _mpeerType<=2 && _mpeerType>=1)
	{
		chnNumber = _chnNumber;
		myPeerId = _mypeerId;
		myPeerType = _mpeerType;
	}
	else
	{
		throw Exception("Invalid Channel Number");
	}
}

SCMHalfChannel::~SCMHalfChannel()
{
	/*stopChannel();

	pthread_mutex_destroy(&runningMutex);
	pthread_mutex_destroy(&haltedMutex);*/
}

long int SCMHalfChannel::processBuffer(unsigned char* buffer, long int _recvSize)
{
	unsigned char* temp_buf=0;
	unsigned long long new_size=0;
	
	SCMProtocol::SCMPacket * scmPacket = new SCMProtocol::SCMPacket(myPeerType,myPeerId,SCMProtocol::Route,chnNumber,buffer,_recvSize);

	temp_buf = scmPacket->getPacket(&new_size);

	for(unsigned long long i=0;i<new_size;i++)
	{
		buffer[i] = temp_buf[i];
	}

	delete scmPacket;
	delete[] temp_buf;

	return (long int) (new_size);
}

SCMRouter::SCMRouter(SCMProtocol::PeerType _mytype, SCMProtocol::PeerType _matchtype, unsigned long long _mypeerId, unsigned long long _matchpeerId, DataSocket* lnkSocket, ServerSocket* appListenSock, unsigned int buffer_size, unsigned int size_overhead, std::string app_ip, unsigned int app_port, unsigned char sync, bool _checkhbeat, bool _blockUserSend)
{
	if(!lnkSocket || buffer_size<SCMProtocol::SCMHeaderSize)
	{
		throw Exception("Invalid Arguments");
	}

	if(!lnkSocket->is_valid())
	{
		throw Exception("Invalid Sockets");
	}

	linkSocket = lnkSocket;
	userListenSocket = appListenSock;
	bufferSize = buffer_size;
	sizeOverhead = size_overhead;
	appIp = app_ip;
	appPort = app_port;
	myPeerType = _mytype;
	matchPeerType = _matchtype;
	myPeerId = _mypeerId;
	matchPeerId = _matchpeerId;
	checkHBeat = _checkhbeat;
	blockUserSend = _blockUserSend;

	for(int i=0;i<SCM_MAX_CHANNELS;i++)
	{
		apacheSockets[i]=0;
		channels[i]=0;
		packetCounter[i]=0;
		incomingRequest[i] = false;
		pthread_mutex_init(&channelMutexes[i],NULL);
	}

	pthread_mutex_init(&hbTimeMutex,NULL);
	pthread_mutex_init(&hbTimeAckMutex,NULL);
	pthread_mutex_init(&waitingHbAckMutex,NULL);
	hbTime=0;
	hbAckTime=0;
	waitingHbAck=false;


	factoryThreadId.id=0;
	factoryThreadId.created=false;
	pthread_mutex_init(&factoryThreadId.id_mutex,NULL);
	auditThreadId.id=0;
	auditThreadId.created=false;
	pthread_mutex_init(&auditThreadId.id_mutex,NULL);
	routerThreadId.id=0;
	routerThreadId.created=false;
	pthread_mutex_init(&routerThreadId.id_mutex,NULL);

	linkUp = true;
	syncChannel = sync;

	pthread_mutex_init(&factoryRunningMutex,NULL);
	pthread_mutex_init(&auditRunningMutex,NULL);
	pthread_mutex_init(&routerRunningMutex,NULL);

	routerRunning=false;
	auditRunning=false;
	factoryRunning=false;

	pthread_mutex_init(&linkSocketMutex,NULL);
}

SCMRouter::~SCMRouter()
{
	pthread_mutex_destroy(&factoryRunningMutex);
	pthread_mutex_destroy(&auditRunningMutex);
	pthread_mutex_destroy(&routerRunningMutex);
	for(int i=0;i<SCM_MAX_CHANNELS;i++)
	{
		pthread_mutex_destroy(&channelMutexes[i]);
	}

	pthread_mutex_destroy(&factoryThreadId.id_mutex);
	pthread_mutex_destroy(&auditThreadId.id_mutex);
	pthread_mutex_destroy(&routerThreadId.id_mutex);

	pthread_mutex_destroy(&hbTimeMutex);
	pthread_mutex_destroy(&hbTimeAckMutex);
	pthread_mutex_destroy(&waitingHbAckMutex);

	pthread_mutex_destroy(&linkSocketMutex);
}

bool SCMRouter::startRouting()
{
	bool started = true;
	started &= startChnFactory();
	started &= startChnAudit();
	started &= startRouter();
	return true;
}

bool SCMRouter::stopRouting()
{
	stopRouter();
	stopChnFactory();
	stopChnAudit();

	for(int i=0;i<SCM_MAX_CHANNELS;i++)
	{
		pthread_mutex_lock(&channelMutexes[i]);
		if(channels[i])
		{
			channels[i]->stopChannel();
			delete channels[i];
			channels[i]=0;
		}
		if(apacheSockets[i])
		{
			delete apacheSockets[i];
			apacheSockets[i]=0;
		}
		pthread_mutex_unlock(&channelMutexes[i]);
	}

	return true;
}

void SCMRouter::waitingHeartBeatAck()
{
	pthread_mutex_lock(&waitingHbAckMutex);
	waitingHbAck=true;
	pthread_mutex_unlock(&waitingHbAckMutex);
}
bool SCMRouter::isLinkUp()
{
	return linkUp;
}

bool SCMRouter::startChnFactory() //Waits for apache connection and creates a channel with the Snitch
{
	stopFactoryThread=false;

	pthread_mutex_lock(&factoryThreadId.id_mutex);
	bool ret = (pthread_create(&factoryThreadId.id, NULL, runChnFactory, this) == 0);

	if(ret)
	{
		factoryThreadId.created=true;
	}

	pthread_mutex_unlock(&factoryThreadId.id_mutex);
	return ret;
}

void SCMRouter::stopChnFactory()
{
	stopFactoryThread=true;


	pthread_mutex_lock(&factoryThreadId.id_mutex);
	if(factoryThreadId.created)
	{
		pthread_join(factoryThreadId.id,NULL);
		factoryThreadId.created = false;
	}

	pthread_mutex_unlock(&factoryThreadId.id_mutex);
}

void* SCMRouter::runChnFactory(void* context)
{
	return ((SCMRouter*)context)->chnFactoryThread();
}

void* SCMRouter::chnFactoryThread()
{
	int i,j;
	DataSocket* newSocket=0;
	SCMProtocol::SCMPacket* scmPacket=0;
	unsigned char * syncPkt=0;
	unsigned long long syncPktSize;
	bool stopFlag = false;
	bool timeoverflow=false;

	pthread_mutex_lock(&factoryRunningMutex);
	factoryRunning=true;
	pthread_mutex_unlock(&factoryRunningMutex);

	if(!userListenSocket)
	{
		return 0;
	}

	for(i=0;i<SCM_MAX_CHANNELS;i++)
	{
		creatingChannel[i] == false;
	}

	try
	{
		while(1)
		{
			while(1)
			{
				if(stopFactoryThread)
				{
					stopFlag=true;
					break;
				}
				j=0;
				for(i=0;i<SCM_MAX_CHANNELS;i++)
				{
					pthread_mutex_lock(&channelMutexes[i]);
					if(channels[i]==0 && apacheSockets[i]==0 && incomingRequest[i]==false && i!=syncChannel)
					{
						pthread_mutex_unlock(&channelMutexes[i]);
						j=1;
						break;
					}
					else
					{
						pthread_mutex_unlock(&channelMutexes[i]);
					}
				}
				if(j)
				{
					break;
				}
				else
				{
					std::cout << "Max Channels Exceeded" << std::endl;
				}
				usleep(500000);
			}
			if(stopFlag)
			{
				break;
			}

			//Synchronizing channel with other router.
			scmPacket = new SCMProtocol::SCMPacket(myPeerType, myPeerId, SCMProtocol::SyncListen, i, NULL, 0);
			syncPkt = scmPacket->getPacket(&syncPktSize);
			pthread_mutex_lock(&linkSocketMutex);
			bool timeover;
			linkSocket->sendData(syncPkt,syncPktSize,6,&timeover);
			pthread_mutex_unlock(&linkSocketMutex);
			delete[] syncPkt;
			syncPkt=0;
			delete scmPacket;
			scmPacket=0;
			syncPktSize = 0;
			//-----------------------------------------

			while(1)
			{
				if(stopFactoryThread)
				{
					stopFlag=true;
					break;
				}
				j=0;
				newSocket = userListenSocket->startListening(500,&timeoverflow); //It doesn't block
				if(timeoverflow)
				{
					newSocket=0;
					continue;
				}
				if(newSocket)
				{
					creatingChannel[i] = true;
					break;
				}
				if(incomingRequest[i])
				{
					incomingRequest[i]=false;
					j=1;
					break;
				}
			}

			if(stopFlag)
			{
				break;
			}

			if(j)
			{
				if(newSocket) //Should never end up here.
				{
					delete newSocket;
				}
				continue;
			}

			pthread_mutex_lock(&channelMutexes[i]);
			apacheSockets[i] = newSocket;
			channels[i] = new SCMHalfChannel(apacheSockets[i],linkSocket,bufferSize,SCMProtocol::SCMHeaderSize,i,myPeerType,myPeerId);
			channels[i]->startChannel();
			creatingChannel[i] = false;
			pthread_mutex_unlock(&channelMutexes[i]);
		}
	}
	catch(Exception &e)
	{
		std::cout << "Factory Thread Exception caught: " << e.description() <<std::endl;
	}

	if(syncPkt)
	{
		delete syncPkt;
		syncPkt=0;
	}
	if(scmPacket)
	{
		delete scmPacket;
		scmPacket=0;
	}
	pthread_mutex_lock(&factoryRunningMutex);
	factoryRunning=false;
	pthread_mutex_unlock(&factoryRunningMutex);
	return 0;
}


bool SCMRouter::startChnAudit()
{
	stopAuditThread=false;


	pthread_mutex_lock(&auditThreadId.id_mutex);
	bool ret = (pthread_create(&auditThreadId.id, NULL, runChnAudit, this) == 0);

	if(ret)
	{
		auditThreadId.created=true;
	}
	
	pthread_mutex_unlock(&auditThreadId.id_mutex);
	return ret;
}
void SCMRouter::stopChnAudit()
{
	stopAuditThread=true;
	pthread_mutex_lock(&auditThreadId.id_mutex);
	if(auditThreadId.created)
	{
		pthread_join(auditThreadId.id,NULL);
		auditThreadId.created = false;
	}

	pthread_mutex_unlock(&auditThreadId.id_mutex);
}
void* SCMRouter::runChnAudit(void* context)
{
	return ((SCMRouter*)context)->chnAuditThread();
}

void* SCMRouter::chnAuditThread()
{
	int i;
	unsigned long long countChn[SCM_MAX_CHANNELS][2];
	unsigned long long countRtr[SCM_MAX_CHANNELS][2];
	unsigned char * syncPkt=0;
	unsigned long long syncPktSize=0;
	SCMProtocol::SCMPacket* scmPacket=0;

	pthread_mutex_lock(&auditRunningMutex);
	auditRunning=true;
	pthread_mutex_unlock(&auditRunningMutex);

	if(!userListenSocket)
	{
		return 0;
	}

	for(i=0;i<SCM_MAX_CHANNELS;i++)
	{
		countChn[i][0]=0;
		countChn[i][1]=0;
		countRtr[i][0]=0;
		countRtr[i][1]=0;
		inactiveTime[i]=0;
		apacheSendFail[i] = false;
	}

	try
	{
		while(1)
		{
			if(stopAuditThread)
			{
				break;
			}

			for(i=0;i<SCM_MAX_CHANNELS;i++)
			{
				pthread_mutex_lock(&channelMutexes[i]);
				if(channels[i]!=0)
				{
					if(apacheSockets[i]==0)
					{
						{
							//Synchronizing channel with other router.
							scmPacket = new SCMProtocol::SCMPacket(myPeerType, myPeerId, SCMProtocol::SyncDelete, i, NULL, 0);
							syncPkt = scmPacket->getPacket(&syncPktSize);
							pthread_mutex_lock(&linkSocketMutex);
							bool timeover;
							linkSocket->sendData(syncPkt,syncPktSize,6, &timeover);
							pthread_mutex_unlock(&linkSocketMutex);
							delete[] syncPkt;
							syncPkt=0;
							delete scmPacket;
							scmPacket=0;
							syncPktSize=0;
							//-----------------------------------------
						}

						std::cout << "Null socket Not null channel " << (unsigned int)i << std::endl;
						channels[i]->stopChannel();
						delete channels[i];
						channels[i]=0;
						packetCounter[i]=0;
						inactiveTime[i]=0;
					}

					countChn[i][0] = channels[i]->getPacketCounter();
					countRtr[i][0] = packetCounter[i];
					if(countChn[i][0]==countChn[i][1] && countRtr[i][0]==countRtr[i][1])
					{
						inactiveTime[i]++;
					}
					else
					{
						inactiveTime[i]=0;
					}
					countChn[i][1]=countChn[i][0];
					countRtr[i][1]=countRtr[i][0];

					if(channels[i]->isHalted() || inactiveTime[i]>=CHANNEL_TIMEOUT || apacheSendFail[i])
					{
						{
							//Synchronizing channel with other router.
							scmPacket = new SCMProtocol::SCMPacket(myPeerType, myPeerId, SCMProtocol::SyncDelete, i, NULL, 0);
							syncPkt = scmPacket->getPacket(&syncPktSize);
							pthread_mutex_lock(&linkSocketMutex);
							bool timeover;
							linkSocket->sendData(syncPkt,syncPktSize,6, &timeover);
							pthread_mutex_unlock(&linkSocketMutex);
							delete[] syncPkt;
							syncPkt=0;
							delete scmPacket;
							scmPacket=0;
							syncPktSize=0;
							//-----------------------------------------
						}

						if(channels[i])
						{
							channels[i]->stopChannel();
							delete channels[i];
							channels[i]=0;
						}
						if(apacheSockets[i]!=0)
						{
							delete apacheSockets[i];
							apacheSockets[i]=0;
						}
						incomingRequest[i]=false;
						apacheSendFail[i]=false;
						packetCounter[i]=0;
						inactiveTime[i]=0;
					}
					pthread_mutex_unlock(&channelMutexes[i]);
				}
				else
				{
					if(apacheSockets[i]!=0)
					{
						std::cout << "Null channel Not null socket " << (unsigned int)i << std::endl;
						delete apacheSockets[i];
						apacheSockets[i]=0;
					}
					pthread_mutex_unlock(&channelMutexes[i]);
				}
			}

			if(checkHBeat)
			{
				pthread_mutex_lock(&hbTimeMutex);
				hbTime++;

				if(hbTime>=35)
				{
					pthread_mutex_unlock(&hbTimeMutex);
					std::cout << "Link up False hbTime>=35" << std::endl;
					linkUp=false;
				}
				else
				{
					pthread_mutex_unlock(&hbTimeMutex);
				}
			}

			pthread_mutex_lock(&waitingHbAckMutex);
			if(waitingHbAck)
			{
				pthread_mutex_unlock(&waitingHbAckMutex);

				pthread_mutex_lock(&hbTimeAckMutex);
				hbAckTime++;
				if(hbAckTime>=10)
				{
					pthread_mutex_unlock(&hbTimeAckMutex);
					std::cout << "Link up False hbAckTime>=10" << std::endl;
					linkUp=false;
				}
				else
				{
					pthread_mutex_unlock(&hbTimeAckMutex);
				}
			}
			else
			{
				pthread_mutex_unlock(&waitingHbAckMutex);
			}

			sleep(1);
		}
	}
	catch(Exception &e)
	{
		std::cout << "Audit Thread Exception caught: " << e.description() <<std::endl;
	}

	if(syncPkt)
	{
		delete syncPkt;
		syncPkt=0;
	}
	if(scmPacket)
	{
		delete scmPacket;
		scmPacket=0;
	}
	pthread_mutex_lock(&auditRunningMutex);
	auditRunning=false;
	pthread_mutex_unlock(&auditRunningMutex);
	return 0;
}


bool SCMRouter::startRouter()
{
	stopRouterThread=false;

	pthread_mutex_lock(&routerThreadId.id_mutex);
	bool ret = (pthread_create(&routerThreadId.id, NULL, runRouter, this) == 0);

	if(ret)
	{
		routerThreadId.created=true;
	}
	
	pthread_mutex_unlock(&routerThreadId.id_mutex);
	return ret;
}
void SCMRouter::stopRouter()
{
	stopRouterThread=true;

	pthread_mutex_lock(&routerThreadId.id_mutex);
	if(routerThreadId.created)
	{
		pthread_join(routerThreadId.id,NULL);
		routerThreadId.created = false;

	}
	pthread_mutex_unlock(&routerThreadId.id_mutex);
}
void* SCMRouter::runRouter(void* context)
{
	return ((SCMRouter*)context)->routerThread();
}
void* SCMRouter::routerThread()
{
	pthread_mutex_lock(&routerRunningMutex);
	routerRunning=true;
	pthread_mutex_unlock(&routerRunningMutex);

	unsigned char* httpBuffer=0;
	unsigned long long analBufferSize=0;
	int recvSize=0;
	int recvErrno;
	unsigned long long sendSize=0;
	unsigned char recvChannel=254;
	bool sendOk=false;
	unsigned int i=0;
	unsigned long long buffCount=0;
	unsigned long long buffMemory= (bufferSize+sizeOverhead) + 100;
	AnalyseReturn analRet;
	unsigned char buffer[buffMemory+50];
	unsigned char analBuffer[4*buffMemory];

	memset(buffer, 0, sizeof(buffer));
	memset(analBuffer, 0, sizeof(analBuffer));

	SCMProtocol::SCMPacket* scmPacket=0;
	//SCMProtocol::PeerType recvType = SCMProtocol::InvalidType;
	SCMProtocol::Command recvCommand= SCMProtocol::InvalidCommand;
	ClientSocket * newApacheSock=0;

	/*buffer = new unsigned char[buffMemory+50]();
	analBuffer = new unsigned char[4*buffMemory]();*/

	waitingHbAck=false;

	for(i=0;i<SCM_MAX_CHANNELS;i++)
	{
		incomingRequest[i] = false;
		packetCounter[i] = 0;
	}

	while(1)
	{
		if(stopRouterThread)
		{
			break;
		}

		try
		{
			recvSize=0;
			recvSize = linkSocket->receiveData(buffer, buffMemory, 500, &recvErrno); //Receiving http with SCM headers

			if(recvSize==-1 && recvErrno!=EWOULDBLOCK && recvErrno!=EAGAIN)
			{
				std::cout << "recvSize==-1 recvErrno==" << recvErrno << std::endl;
				linkUp = false;
				//socket error
				break;
			}
			else if(recvSize==0)
			{
				std::cout << "recvSize==0" << std::endl;
				//Connection Loss
				linkUp = false;
				break;
			}
			else if(recvSize>0)
			{
				//linkUp = true;

				//analBuffer[] = analBuffer[] + buffer[]
				for(buffCount=0;buffCount<(unsigned long long)recvSize;buffCount++)
				{
					analBuffer[buffCount+analBufferSize] = buffer[buffCount];
				}

				analBufferSize += recvSize;
				analRet.closedPacketNum = 0;
				analRet.allBytesClosed = false;
				for(i=0;i<MAX_SCM_PACKETS;i++)
				{
					analRet.closedIndexes[i]=0;
					analRet.closedLengths[i]=0;
				}

				SCMProtocol::SCMPacket::analysePacket(matchPeerType, analBuffer, analBufferSize, &analRet);

				for(i=0;i<analRet.closedPacketNum;i++)
				{
					if(i>MAX_SCM_PACKETS)
					{
						break; //Ignoring packets outside the SCM limits.
					}
					//Parsing and Sending ONE Closed packet

					if(analRet.closedLengths[i]==0 || analRet.closedLengths[i]>(4*buffMemory) || analRet.closedIndexes[i]>(4*buffMemory))
					{
						std::cout << "Invalid Length/Index -> index[" << i << "]: " << analRet.closedIndexes[i] << " length[" << i << "]: " << analRet.closedLengths[i] << std::endl;
						continue;
					}

					scmPacket = new SCMProtocol::SCMPacket();

					if(!scmPacket->setPacket(&(analBuffer[analRet.closedIndexes[i]]),analRet.closedLengths[i]))
					{
						if(scmPacket)
						{
							delete scmPacket;
							scmPacket=0;
						}
						std::cout << "Error setting packet." << std::endl;
						continue;
					}
					if(!scmPacket->isValid())
					{
						if(scmPacket)
						{
							delete scmPacket;
							scmPacket=0;
						}
						std::cout << "Invalid scmPacket" << std::endl;
						continue;
					}
					

					//Implement this check just when all snitches have been updated and stable
					/*if(matchPeerType != scmPacket->getPeerType())
					{
						std::cout << "Invalid MatchPeerType" << std::endl;
						continue;
					}
					if(matchPeerId != scmPacket->getPeerId())
					{
						std::cout << "Invalid MatchPeerId" << std::endl;
						continue;
					}*/
					//-------------------------------------------------------------------------

					recvCommand = scmPacket->getCommand();
					recvChannel = scmPacket->getChannel();


					if(recvCommand==SCMProtocol::SyncListen)
					{
						syncChannel = recvChannel;
						if(scmPacket)
						{
							delete scmPacket;
							scmPacket=0;
						}
						continue;
					}

					if(recvCommand==SCMProtocol::SyncDelete)
					{
						pthread_mutex_lock(&channelMutexes[recvChannel]);
						if(channels[recvChannel])
						{
							channels[recvChannel]->stopChannel();
							delete channels[recvChannel];
							incomingRequest[recvChannel] = false;
							channels[recvChannel]=0;
							packetCounter[recvChannel]=0;
							inactiveTime[i]=0;
						}
						if(apacheSockets[recvChannel])
						{
							delete apacheSockets[recvChannel];
							apacheSockets[recvChannel]=0;
						}
						pthread_mutex_unlock(&channelMutexes[recvChannel]);
						sched_yield();

						if(scmPacket)
						{
							delete scmPacket;
							scmPacket=0;
						}
						continue;
					}

					if(recvCommand==SCMProtocol::HeartBeat)
					{
						pthread_mutex_lock(&hbTimeMutex);
						hbTime=0;
						pthread_mutex_unlock(&hbTimeMutex);

						{
							SCMProtocol::SCMPacket* ackPacket = new SCMProtocol::SCMPacket(SCMProtocol::Server,1,SCMProtocol::HeartBeatAck,0,NULL,0);
							unsigned long long size=0;
							unsigned char * pakt = ackPacket->getPacket(&size);

							if(pakt && size>0)
							{
								pthread_mutex_lock(&linkSocketMutex);
								bool timeover;
								linkSocket->sendData(pakt,(size_t)size,0,&timeover);
								pthread_mutex_unlock(&linkSocketMutex);
								//std::cout << "Sent HeartBeat Ack" << std::endl;
							}
							
							if(ackPacket)
							{
								delete ackPacket;
								ackPacket=0;
							}
							if(pakt)
							{
								delete[] pakt;
								pakt=0;
							}
							size=0;
						}

						if(scmPacket)
						{
							delete scmPacket;
							scmPacket=0;
						}
						continue;
					}

					pthread_mutex_lock(&waitingHbAckMutex);
					if(recvCommand==SCMProtocol::HeartBeatAck && waitingHbAck)
					{
						waitingHbAck=false;
						pthread_mutex_unlock(&waitingHbAckMutex);

						pthread_mutex_lock(&hbTimeAckMutex);
						hbAckTime=0;
						pthread_mutex_unlock(&hbTimeAckMutex);
						if(scmPacket)
						{
							delete scmPacket;
							scmPacket=0;
						}
						continue;

					}
					else
					{
						pthread_mutex_unlock(&waitingHbAckMutex);
					}

					if(recvCommand!=SCMProtocol::Route)
					{
						if(scmPacket)
						{
							delete scmPacket;
							scmPacket=0;
						}
						std::cout << "Invalid Arguments" << std::endl;
						continue;
					}

					pthread_mutex_lock(&channelMutexes[recvChannel]);
					if(channels[recvChannel]==0 && apacheSockets[recvChannel]==0)
					{
						if(myPeerType==SCMProtocol::Server)
						{
							if(scmPacket)
							{
								delete scmPacket;
								scmPacket=0;
							}
							//std::cout << "Blocking server from creating a new channel" << std::endl;
							pthread_mutex_unlock(&channelMutexes[recvChannel]);
							sched_yield();
							continue;
						}

						incomingRequest[recvChannel]=true;
						newApacheSock = new ClientSocket();
						if(!newApacheSock)
						{
							pthread_mutex_unlock(&channelMutexes[recvChannel]);
							sched_yield();
							continue;
						}
						if(!newApacheSock->connectTo(appIp,appPort))
						{
							if(newApacheSock)
							{
								delete newApacheSock;
								newApacheSock=0;
							}
							if(scmPacket)
							{
								delete scmPacket;
								scmPacket=0;
							}
							pthread_mutex_unlock(&channelMutexes[recvChannel]);
							sched_yield();
							continue;
						}

						if(creatingChannel[recvChannel])
						{
							if(newApacheSock)
							{
								delete newApacheSock;
								newApacheSock=0;
							}
							if(scmPacket)
							{
								delete scmPacket;
								scmPacket=0;
							}
							pthread_mutex_unlock(&channelMutexes[recvChannel]);
							sched_yield();
							continue;
						}

						apacheSockets[recvChannel] = newApacheSock;
						channels[recvChannel] = new SCMHalfChannel(apacheSockets[recvChannel],linkSocket,bufferSize,SCMProtocol::SCMHeaderSize,recvChannel, myPeerType, myPeerId);
						channels[recvChannel]->startChannel();
						pthread_mutex_unlock(&channelMutexes[recvChannel]);
						sched_yield();
					}
					else
					{
						pthread_mutex_unlock(&channelMutexes[recvChannel]);
						sched_yield();
					}

					httpBuffer = scmPacket->getPayload(&sendSize);
					sendOk = true;

					if(!httpBuffer || !sendSize)
					{
						if(httpBuffer)
						{
							delete[] httpBuffer;
						}
						httpBuffer=0;
						sendSize=0;
						if(scmPacket)
						{
							delete scmPacket;
							scmPacket=0;
						}
						std::cout << "Null Payload" << std::endl;
						continue;
					}

					pthread_mutex_lock(&channelMutexes[recvChannel]);

					if(apacheSockets[recvChannel])
					{
						bool timeover;

						/*REMOVING OMNICOMM WORD*/
						{
							int index=0;
							for(int k=0;k<sendSize;k++)
							{
								if(index==0 && (httpBuffer[k]=='o' || httpBuffer[k]=='O'))
								{
									index++;
								}
								else if(index==1 && (httpBuffer[k]=='m' || httpBuffer[k]=='M'))
								{
									index++;
								}
								else if(index==2 && (httpBuffer[k]=='n' || httpBuffer[k]=='N'))
								{
									index++;
								}
								else if(index==3 && (httpBuffer[k]=='i' || httpBuffer[k]=='I'))
								{
									index++;
								}
								else if(index==4 && (httpBuffer[k]=='c' || httpBuffer[k]=='C'))
								{
									index++;
								}
								else if(index==5 && (httpBuffer[k]=='o' || httpBuffer[k]=='O'))
								{
									index++;
								}
								else if(index==6 && (httpBuffer[k]=='m' || httpBuffer[k]=='M'))
								{
									index++;
								}
								else if(index==7 && (httpBuffer[k]=='m' || httpBuffer[k]=='M'))
								{
									for(int m=7;m>=0;m--)
									{
										httpBuffer[k-m]='x';
									}
									index=0;
								}
								else
								{
									index=0;
								}
							}
						}

						if(blockUserSend)
						{
							sendOk &= apacheSockets[recvChannel]->sendData(httpBuffer,sendSize,3,&timeover); //Sending raw http to right channel
						}
						else
						{
							sendOk &= apacheSockets[recvChannel]->sendData(httpBuffer,sendSize,3,&timeover); //Sending raw http to right channel
						}
						if(timeover)
						{
							std::cout << "Channel " << (unsigned int)recvChannel << " Timed Over " << (unsigned int)sendOk << std::endl;
						}

						apacheSendFail[recvChannel] = !(sendOk);
						packetCounter[recvChannel]++;

						pthread_mutex_unlock(&channelMutexes[recvChannel]);
						sched_yield();

						if(httpBuffer)
						{
							delete[] httpBuffer;
							httpBuffer=0;
						}
						sendSize=0;
						if(scmPacket)
						{
							delete scmPacket;
							scmPacket=0;
						}
					}
					else
					{
						apacheSendFail[recvChannel] = true;
						pthread_mutex_unlock(&channelMutexes[recvChannel]);
						sched_yield();
					}
				}

				if(analRet.closedPacketNum>0)
				{
					if(analRet.allBytesClosed)
					{
						analBufferSize=0;
					}
					else
					{
						unsigned long long forIndex = analRet.closedIndexes[analRet.closedPacketNum-1] + analRet.closedLengths[analRet.closedPacketNum-1];
						unsigned long long forSize = analBufferSize - forIndex;

						if(forSize>0 && forSize<=4*buffMemory && forIndex>0 && forIndex<=4*buffMemory)
						{
							for(unsigned long long j=0;j<forSize;j++)
							{
								analBuffer[j+analRet.closedIndexes[0]] = analBuffer[j+forIndex];
							}
							analBufferSize = forSize+analRet.closedIndexes[0];
						}
						else
						{
							/*std::cout << "forSize: " << forSize << std::endl; 
							std::cout << "forIndex: " << forIndex << std::endl;
							std::cout << "analBufferSize: " << analBufferSize <<std::endl;
							std::cout << "lastIndex: " << analRet.closedIndexes[analRet.closedPacketNum-1] << std::endl;
							std::cout << "lastLength: " << analRet.closedLengths[analRet.closedPacketNum-1] << std::endl;
							std::cout << "firstIndex: " << analRet.closedIndexes[0] << std::endl;*/

							if(analBufferSize>3*buffMemory)
							{
								std::cout << "Dropped: " << analBufferSize << " bytes - Snitch " << matchPeerId << std::endl;
								analBufferSize=0;
							}
							else
							{
								analBufferSize=analRet.closedIndexes[0];
							}
						}
					}
				}
				else
				{
					if(analBufferSize>3*buffMemory)
					{
						std::cout << "Dropped: " << analBufferSize << " bytes - Snitch " << matchPeerId << " 2" << std::endl;
						analBufferSize=0;
					}
				}

			}
			else
			{
				//No data received
			}
		}
		catch(Exception &e)
		{
			std::cout << "Router Thread Exception caught: " << e.description() <<std::endl;
		}
	}

	/*if(buffer)
	{
		delete[] buffer;
		buffer=0;
	}
	
	if(analBuffer)
	{
		delete[] analBuffer;
		analBuffer=0;
	}*/

	if(httpBuffer)
	{
		delete[] httpBuffer;
		httpBuffer=0;
	}
	if(scmPacket)
	{
		delete scmPacket;
		scmPacket=0;
	}
	
	pthread_mutex_lock(&routerRunningMutex);
	routerRunning=false;
	pthread_mutex_unlock(&routerRunningMutex);

	return 0;
}