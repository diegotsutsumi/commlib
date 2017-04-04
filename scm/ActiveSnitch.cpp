#include "ActiveSnitch.h"
#include "Exception.h"
#include "SCMPacket.h"
#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <sys/stat.h>
#include "globals.h"

using namespace SCMProtocol;

ActiveSnitch::ActiveSnitch(std::string _filePath, bool _routerSendBlock)
{
	port=0;
	snitchSerialNum=0;
	snitchSocket=0;
	apacheListenSocket=0;
	linkUp=false;
	mainRouter=0;

	lnkAuditThreadId.created=false;
	lnkAuditThreadId.id=0;
	pthread_mutex_init(&lnkAuditThreadId.id_mutex,NULL);

	routerSendBlock = _routerSendBlock;
	filePath = _filePath;
	routeEnable=false;
	stopAudit=false;
	deleteOnDestructor=false;
	pthread_mutex_init(&routerMutex,NULL);
	pthread_mutex_init(&stopAuditMutex,NULL);
}

ActiveSnitch::~ActiveSnitch()
{
	if(apacheListenSocket!=0)
	{
		delete apacheListenSocket;
	}

	pthread_mutex_lock(&routerMutex);
	if(mainRouter)
	{
		mainRouter->stopRouting();
		delete mainRouter;
		mainRouter=0;
	}
	pthread_mutex_unlock(&routerMutex);

	if(deleteOnDestructor)
	{
		deleteDb();
	}

	pthread_mutex_destroy(&routerMutex);
	pthread_mutex_destroy(&stopAuditMutex);
	pthread_mutex_destroy(&lnkAuditThreadId.id_mutex);
}

bool ActiveSnitch::startSnitch(DataSocket* snitch_sock, unsigned int p)
{
	std::ifstream inputFile(filePath);
	std::ofstream outputFile;
	unsigned int snitchNum=0;
	bool alreadyExists=false;
	std::string line;
	bool ret=false;

	unsigned char* snPacket=0;
	unsigned long long snPacketLen=0;

	if(!snitch_sock || !p)
	{
		return false;
	}

	try
	{
		this->snitchSocket = snitch_sock;
		snitchSerialNum = getSerialNum();

		if(snitchSerialNum!=0)
		{
			//TODO: Insert Serial Number + Port into DB.

			pthread_mutex_lock(&snitchesFileMutex);
			if (inputFile.is_open())
			{
				while ( getline (inputFile,line) )
				{
					try
					{
						snitchNum = std::stoi(line);
						if(snitchNum == snitchSerialNum)
						{
							alreadyExists=true;
							break;
						}
					}
					catch (const std::invalid_argument& ia)
					{
						std::cerr << "ActiveSnitch 1 Invalid argument STOI: " << ia.what() << '\n';
						//inputFile.close();
						//return false;
					}
				}
				inputFile.close();
			}
			else
			{
				std::cerr << "Couldn't open the input file" << '\n';
			}

			if(!alreadyExists)
			{
				this->port = p+snitchSerialNum; // Temporary
				this->apacheListenSocket = new ServerSocket(this->port);
				outputFile.open(filePath,std::ios_base::app);
				outputFile << snitchSerialNum << std::endl;
				outputFile.close();
				pthread_mutex_unlock(&snitchesFileMutex);



				linkUp = true;
				startLinkAudit();
				ret = true;
				deleteOnDestructor=true;
			}
			else
			{
				pthread_mutex_unlock(&snitchesFileMutex);
				ret = false;
			}
		}
		else
		{
			ret=false;
		}

		SCMPacket serialPacket(SCMProtocol::Server, 1, ret?Ack:Nack, 0,0,0);
		serialPacket.clearPayload();
		snPacket = serialPacket.getPacket(&snPacketLen);
		if(snPacket && snPacketLen>0)
		{
			bool timeover;
			snitchSocket->sendData(snPacket,(size_t)snPacketLen, 0, &timeover);
		}
		else
		{
			delete[] snPacket;
			throw Exception("Packet mount error.");
		}

		delete[] snPacket;
		snPacketLen=0;

		if(ret)
		{
			pthread_mutex_lock(&routerMutex);
			mainRouter = new SCMRouter(SCMProtocol::Server,SCMProtocol::Client,1,snitchSerialNum,snitchSocket,apacheListenSocket,4096, SCMProtocol::SCMHeaderSize,"127.0.0.1",443,1,true,routerSendBlock); //appIp and appPort is just figurative
			mainRouter->startRouting();
			pthread_mutex_unlock(&routerMutex);
		}

		return ret;

	}
	catch(Exception &e)
	{
		std::cout<<"ActiveSnitch 2 Exception Caught: " << e.description() << std::endl;
		return false;
	}
}

void ActiveSnitch::stopSnitch()
{
	stopLinkAudit();

	pthread_mutex_lock(&routerMutex);
	if(mainRouter)
	{
		mainRouter->stopRouting();
		delete mainRouter;
		mainRouter=0;
	}
	pthread_mutex_unlock(&routerMutex);
}


bool ActiveSnitch::startLinkAudit()
{
	pthread_mutex_lock(&lnkAuditThreadId.id_mutex);
	bool ret = (pthread_create(&lnkAuditThreadId.id, NULL, runLinkAudit, this) == 0);

	if(ret)
	{
		lnkAuditThreadId.created=true;
	}

	pthread_mutex_unlock(&lnkAuditThreadId.id_mutex);
	return ret;
}

bool ActiveSnitch::stopLinkAudit()
{
	pthread_mutex_lock(&stopAuditMutex);
	stopAudit=true;
	pthread_mutex_unlock(&stopAuditMutex);

	pthread_mutex_lock(&lnkAuditThreadId.id_mutex);
	if(lnkAuditThreadId.created)
	{
		pthread_join(lnkAuditThreadId.id,NULL);
		lnkAuditThreadId.created=false;
	}

	pthread_mutex_unlock(&lnkAuditThreadId.id_mutex);

	return true;
}

void* ActiveSnitch::runLinkAudit(void* context)
{
	return ((ActiveSnitch*)context)->linkAuditThread();
}

void* ActiveSnitch::linkAuditThread()
{
	while(1)
	{
		sleep(1);

		pthread_mutex_lock(&stopAuditMutex);
		if(stopAudit)
		{
			pthread_mutex_unlock(&stopAuditMutex);
			break;
		}
		pthread_mutex_unlock(&stopAuditMutex);

		pthread_mutex_lock(&routerMutex);
		linkUp = mainRouter->isLinkUp();
		pthread_mutex_unlock(&routerMutex);
	}

	return 0;
}

void ActiveSnitch::insertDb()
{
	//TODO
}

void ActiveSnitch::deleteDb()
{
	std::ifstream inputFile(filePath);
	std::ofstream outputFile;
	std::string line;
	std::string tmpFile = filePath + "_tmp_" + std::to_string(snitchSerialNum);
	std::string cmd;
	unsigned int snitchNum;
	bool createdFileFlag=false;

	pthread_mutex_lock(&snitchesFileMutex);

	if (inputFile.is_open())
	{
		while ( getline (inputFile,line) )
		{
			try
			{
				snitchNum = std::stoi(line);

				if(snitchNum!=snitchSerialNum)
				{
					outputFile.open(tmpFile,std::ios_base::app);
					outputFile << snitchNum << std::endl;
					outputFile.close();
					createdFileFlag=true;
				}
			}
			catch (const std::invalid_argument& ia)
			{
				std::cerr << "ActiveSnitch 3 Invalid argument STOI: " << ia.what() << '\n';
				//inputFile.close();
			}
		}
	}
	else
	{
		std::cerr << "Couldn't open the input file" << '\n';
	}
	inputFile.close();

	if(!createdFileFlag)
	{
		outputFile.open(tmpFile,std::ios_base::app);
		outputFile.close();
	}

	rename(tmpFile.c_str(),filePath.c_str());
	cmd = "sudo chmod 755 " + filePath;
	system(cmd.c_str());

	pthread_mutex_unlock(&snitchesFileMutex);
}

bool ActiveSnitch::isLinkUp()
{
	return linkUp;
}

unsigned long long ActiveSnitch::getSerialNumber()
{
	return snitchSerialNum;
}

unsigned long long ActiveSnitch::getSerialNum()
{
	try
	{
		unsigned char * snPacket = new unsigned char[30+1]();
		int snPacketLen=0;
		unsigned long long snitchId;
		bool validInfo=true;
		int _errno=0;
		int recvWaitNum=0;

		while(1)
		{
			if(recvWaitNum>4)
			{
				validInfo=false;
				break;
			}

			snPacketLen = snitchSocket->receiveData(snPacket,SCMHeaderSize,500,&_errno);
			
			if(snPacketLen==-1 && _errno!=EWOULDBLOCK && _errno!=EAGAIN)
			{
				validInfo=false;
				break;
				//connectionActive = false;
				//socket error
				//break;
			}
			else if(snPacketLen==0)
			{
				validInfo=false;
				break;
				//Connection Loss
				//connectionActive = false;
				//break;
			}
			else if(snPacketLen>0)
			{
				recvWaitNum=0;
				break;
			}
			else
			{
				recvWaitNum++;
				//No data received
			}
		}
		

		if(!validInfo)
		{
			return 0;
		}
		
		SCMPacket serialPacket(snPacket,snPacketLen);
		delete[] snPacket;
		snPacket=0;
		snPacketLen=0;

		snitchId = serialPacket.getPeerId();

		if(!serialPacket.isValid())
		{
			validInfo=false;
		}
		if(serialPacket.getCommand()!=Register)
		{
			validInfo=false;
		}

		return validInfo?snitchId:0;
	}
	catch(Exception &e)
	{
		throw e;
	}
}