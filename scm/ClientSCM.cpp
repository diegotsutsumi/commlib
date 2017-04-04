#include "ClientSCM.h"
#include "Exception.h"
#include "SCMPacket.h"
#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>

using namespace SCMProtocol;

ClientSCM::ClientSCM(AppType _type, std::string _appIp, unsigned short _appPort)
{
	try
	{
		pthread_mutex_init(&stopHBMutex,NULL);
		pthread_mutex_init(&hBThreadId.id_mutex,NULL);
		hBThreadId.created=false;
		hBThreadId.id=0;

		type=_type;
		appIp=_appIp;
		appPort=_appPort;
		stopHB = false;
		mainRouter = 0;
		serverLinkSocket = 0;
		serialNumber=0; //invalid.
	}
	catch(Exception &e)
	{
		//std::cout << "Exception caught: " << e.description() << std::endl;
		throw SocketException(e.description());
	}
}

ClientSCM::~ClientSCM()
{
	if(mainRouter)
	{
		mainRouter->stopRouting();
		delete mainRouter;
	}

	if(serverLinkSocket)
		delete serverLinkSocket;

	pthread_mutex_destroy(&stopHBMutex);
	pthread_mutex_destroy(&hBThreadId.id_mutex);
}

bool ClientSCM::runClientSCM()
{
	unsigned char * pakt;
	unsigned long long size;
	int _errno;
	std::string line;

	try
	{
		serverLinkSocket = new ClientSocket();
		int serverPort=0;
		int clientAppPort=0;
		std::string clientAppIp="";
		switch(type)
		{
			case https_type:
				serverPort=DEFAULT_HTTPS_SERVER_PORT;
				clientAppIp="127.0.0.1";
				clientAppPort=60001;
				break;
			case ssh_type:
				serverPort=DEFAULT_SSH_SERVER_PORT;
				clientAppIp="127.0.0.1";
				clientAppPort=60002;
				break;
			case rtsp_type:
				serverPort=DEFAULT_RTSP_SERVER_PORT;
				clientAppIp=appIp;
				clientAppPort=appPort;
				break;
			default:
				serverPort=DEFAULT_HTTPS_SERVER_PORT;
				clientAppIp=appIp;
				clientAppPort=appPort;
				break;
		}

		if(!serverLinkSocket->connectTo("127.0.0.1",serverPort))
		{
			std::cout << "Couldn't connect to server" << std::endl;
			return false;
		}

		serialNumber=1;

		SCMProtocol::SCMPacket *scmPacket = new SCMProtocol::SCMPacket(SCMProtocol::Client,serialNumber,SCMProtocol::Register,0,NULL,0);

		pakt = scmPacket->getPacket(&size);

		if(pakt && size>0)
		{
			usleep(500000);
			bool timeover;
			serverLinkSocket->sendData(pakt,size,0,&timeover);
		}
		else
		{
			if(pakt)
				delete[] pakt;
			throw Exception("Packet Error.");
		}

		delete[] pakt;
		size=0;

		pakt = new unsigned char[SCMHeaderSize];

		size = serverLinkSocket->receiveData(pakt,SCMHeaderSize, 0,&_errno); //TODO: Missing post timeout handling

		if(size>0)
		{
			if(!scmPacket->setPacket(pakt,size))
			{
				return false;
			}
			if(scmPacket->getCommand()==SCMProtocol::Ack)
			{
				std::cout << "Received Ack from Server" << std::endl;
				//std::cout << "Received Ack from Server" << std::endl;
			}
			else if(scmPacket->getCommand()==SCMProtocol::Nack)
			{
				std::cout << "Received Nack from Server" << std::endl;
				delete scmPacket;
				delete[] pakt;
				scmPacket=0;
				pakt=0;
				return false;
			}
			else
			{
				std::cout << "Waiting for Ack/Nack and received: " << scmPacket->getCommand() << std::endl;
				delete scmPacket;
				delete[] pakt;
				scmPacket=0;
				pakt=0;
				return false;
			}
		}
		else
		{
			delete scmPacket;
			delete[] pakt;
			scmPacket=0;
			pakt=0;
			throw Exception("Couldn't receive packet, socket could be shut down");
		}

		delete scmPacket;
		delete[] pakt;
		scmPacket=0;
		pakt=0;
		size=0;


 		mainRouter = new SCMRouter(SCMProtocol::Client,SCMProtocol::Server, serialNumber,1,serverLinkSocket,NULL,4096,SCMProtocol::SCMHeaderSize, clientAppIp, clientAppPort,0,false,false);
 		mainRouter->startRouting();

		//HeartBeat Thread
		startHeartBeat();

		while(1)
		{
			sleep(3);
			if(!mainRouter->isLinkUp())
			{
				break;
			}
		}

		stopHeartBeat();

		mainRouter->stopRouting();
		delete mainRouter;
		mainRouter=0;

		std::cout << "Link with server is Down" << std::endl;

		return true;
	}
	catch(Exception &e)
	{
		std::cout << "Exception Caught : " << e.description() << std::endl;
		return false;
	}

	return false;
}

bool ClientSCM::startHeartBeat()
{
	pthread_mutex_lock(&hBThreadId.id_mutex);
	bool ret = (pthread_create(&hBThreadId.id, NULL, runHeartBeat, this) == 0);

	if(ret)
	{
		hBThreadId.created=true;
	}

	pthread_mutex_unlock(&hBThreadId.id_mutex);
	return ret;
}
bool ClientSCM::stopHeartBeat()
{
	pthread_mutex_lock(&stopHBMutex);
	stopHB=true;
	pthread_mutex_unlock(&stopHBMutex);

	pthread_mutex_lock(&hBThreadId.id_mutex);
	if(hBThreadId.created)
	{
		pthread_join(hBThreadId.id,NULL);
		hBThreadId.created=false;
	}

	pthread_mutex_unlock(&hBThreadId.id_mutex);

	return true;
}
void* ClientSCM::runHeartBeat(void* context)
{
	return ((ClientSCM*)context)->heartBeatThread();
}
void* ClientSCM::heartBeatThread()
{
	unsigned int i=0;
	bool retrn=false;
	SCMProtocol::SCMPacket *scmPacket=0;
	unsigned char * pakt=0;
	unsigned long long size=0;
	
	while(1)
	{
		i=0;
		retrn = false;
		while(1)
		{
			pthread_mutex_lock(&stopHBMutex);
			if(stopHB)
			{
				retrn = true;
				pthread_mutex_unlock(&stopHBMutex);
				break;
			}
			pthread_mutex_unlock(&stopHBMutex);

			if(i>=20)
			{
				break;
			}

			sleep(1);
			i++;
		}

		if(retrn)
		{
			break;
		}

		i=0;

		//SendHeartBeat;

		scmPacket = new SCMProtocol::SCMPacket(SCMProtocol::Client,serialNumber,SCMProtocol::HeartBeat,0,NULL,0);
		pakt = scmPacket->getPacket(&size);

		if(pakt && size>0)
		{
			bool timeover;
			serverLinkSocket->sendData(pakt,(size_t)size,0,&timeover);
			//std::cout << "Sent HeartBeat" << std::endl;
			mainRouter->waitingHeartBeatAck();
		}
		
		if(scmPacket)
		{
			delete scmPacket;
			scmPacket=0;
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
	if(pakt)
	{
		delete[] pakt;
		pakt=0;
	}
	size=0;
	return 0;
}