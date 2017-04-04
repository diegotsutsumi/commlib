#include "ServerSCM.h"
#include "Exception.h"
#include "SCMPacket.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <sys/stat.h>
#include "globals.h"

using namespace std;

pthread_mutex_t snitchesFileMutex = PTHREAD_MUTEX_INITIALIZER;

ServerSCM::ServerSCM(AppType _type)
{
	std::fstream outputFile;
	std::string cmd;
	this->type = _type;
	server = 0;
	for(int i=0; i<MAX_SNITCHES; i++)
	{
		activeSnitches[i]=0;
		linkSockets[i]=0;
		pthread_mutex_init(&snitchMutexes[i],NULL);
	}
	filePath="";

	switch(type)
	{
		case https_type:
			filePath = "/var/scm/active_snitches_web";
			serverPort=DEFAULT_HTTPS_SERVER_PORT;
			baseClientPort=40000;
			break;
	
		case ssh_type:
			filePath = "/var/scm/active_snitches_ssh";
			serverPort=DEFAULT_SSH_SERVER_PORT;
			baseClientPort=20000;
			break;
	
		case rtsp_type:
			filePath = "/var/scm/active_snitches_rtsp";
			serverPort=DEFAULT_RTSP_SERVER_PORT;
			baseClientPort=10000;
			break;
			
		default: //Should never fall here
			filePath = "/var/scm/active_snitches_web";
			serverPort=DEFAULT_HTTPS_SERVER_PORT;
			baseClientPort=40000;
			break;
	}

	//pthread_mutex_init(&snitchesFileMutex,NULL);

	pthread_mutex_lock(&snitchesFileMutex);
	remove(filePath.c_str());
	outputFile.open(filePath,std::fstream::out|std::fstream::trunc);
	outputFile.close();
	cmd = "sudo chmod 755 " + filePath;
	int ret = system(cmd.c_str());

	cout << "ret: " << ret << endl;
	//chmod(filePath.c_str(),755);
	pthread_mutex_unlock(&snitchesFileMutex);

	snAuditThread.id=0;
	snAuditThread.created = false;
	pthread_mutex_init(&snAuditThread.id_mutex,NULL);
	stopServer=false;
	pthread_mutex_init(&stopMutex,NULL);
	
	startSnitchAudit();
}

ServerSCM::~ServerSCM()
{
	//stopSnitchAudit();
	if(server)
	{
		delete server;
	}

	for(int i=0; i<MAX_SNITCHES; i++)
	{
		pthread_mutex_lock(&snitchMutexes[i]);
		if(activeSnitches[i])
		{
			activeSnitches[i]->stopSnitch();
			delete activeSnitches[i];
		}
		if(linkSockets[i])
		{
			delete linkSockets[i];
		}
		pthread_mutex_unlock(&snitchMutexes[i]);
	}

	for(int i=0; i<MAX_SNITCHES;i++)
	{
		pthread_mutex_destroy(&snitchMutexes[i]);
	}

	pthread_mutex_destroy(&stopMutex);
	pthread_mutex_destroy(&snAuditThread.id_mutex);
}

bool ServerSCM::startServerSCM()
{
	unsigned int i=0,j=0;
	bool timeoverflow;
	try
	{
		server = new ServerSocket(serverPort);
		while(1)
		{
			while(1)
			{
				j=0;
				for(i=0;i<MAX_SNITCHES;i++)
				{
					pthread_mutex_lock(&snitchMutexes[i]);
					if(linkSockets[i]==0 && activeSnitches[i]==0)
					{
						j=1;
						pthread_mutex_unlock(&snitchMutexes[i]);
						break;
					}
					pthread_mutex_unlock(&snitchMutexes[i]);
				}
				if(j)
				{
					break;
				}
				usleep(500000);
			}

			linkSockets[i] = server->startListening(0,&timeoverflow); //It Blocks

			if(!linkSockets[i])
			{
				cout << "Server returned zero." << endl;
				continue;
			}

			pthread_mutex_lock(&snitchMutexes[i]);
			activeSnitches[i] = new ActiveSnitch(filePath,(type!=rtsp_type));
			if(!activeSnitches[i]->startSnitch(linkSockets[i],baseClientPort))
			{
				activeSnitches[i]->stopSnitch();
				delete activeSnitches[i];
				delete linkSockets[i];
				activeSnitches[i]=0;
				linkSockets[i]=0;
			}
			else
			{
				if(activeSnitches[i]->getSerialNumber()==0)
				{
					activeSnitches[i]->stopSnitch();
					delete activeSnitches[i];
					delete linkSockets[i];
					activeSnitches[i]=0;
					linkSockets[i]=0;
				}
				else
				{
				}
			}
			pthread_mutex_unlock(&snitchMutexes[i]);
		}
	}
	catch (SocketException& e)
	{
		cout << "Exception was caught:" << e.description() << "\nExiting.\n";
	}
	return false;
}

bool ServerSCM::stopServerSCM()
{
	return stopSnitchAudit();
}

bool ServerSCM::startSnitchAudit()
{
	pthread_mutex_lock(&snAuditThread.id_mutex);
	bool ret = (pthread_create(&snAuditThread.id, NULL, runSnitchAudit, this) == 0);

	if(ret)
	{
		snAuditThread.created=true;
	}

	pthread_mutex_unlock(&snAuditThread.id_mutex);
	return ret;
}
bool ServerSCM::stopSnitchAudit()
{
	pthread_mutex_lock(&stopMutex);
	stopServer=true;
	pthread_mutex_unlock(&stopMutex);


	pthread_mutex_lock(&snAuditThread.id_mutex);
	if(snAuditThread.created)
	{
		pthread_join(snAuditThread.id,NULL);
		snAuditThread.created = false;
	}

	pthread_mutex_unlock(&snAuditThread.id_mutex);

	return true;
}
void* ServerSCM::runSnitchAudit(void* context)
{
	return ((ServerSCM*)context)->snitchAuditThread();
}
void* ServerSCM::snitchAuditThread()
{
	unsigned int i=0;
	while(1)
	{
		sleep(1);
		pthread_mutex_lock(&stopMutex);
		if(stopServer)
		{
			pthread_mutex_unlock(&stopMutex);
			break;
		}
		
		pthread_mutex_unlock(&stopMutex);

		for(i=0;i<MAX_SNITCHES;i++)
		{
			pthread_mutex_lock(&snitchMutexes[i]);
			if(activeSnitches[i])
			{
				if(!activeSnitches[i]->isLinkUp() || !linkSockets[i])
				{
					activeSnitches[i]->stopSnitch();
					delete activeSnitches[i];
					activeSnitches[i]=0;
					if(linkSockets[i])
					{
						delete linkSockets[i];
						linkSockets[i]=0;
					}
				}
			}
			else
			{
				if(linkSockets[i])
				{
					delete linkSockets[i];
					linkSockets[i]=0;
				}
			}
			pthread_mutex_unlock(&snitchMutexes[i]);
		}
	}
	return 0;
}