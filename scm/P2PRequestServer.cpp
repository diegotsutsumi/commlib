#include "P2PRequestServer.h"
#include "Exception.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include "globals.h"

using namespace std;

P2PRequestServer::P2PRequestServer(unsigned int _port)
{
	server = 0;
	for(int i=0; i<MAX_P2P_CONNECTIONS; i++)
	{
		connections[i]=0;
		connectionsRet[i]=Return_Nothing;
		pthread_mutex_init(&connectionsMutex[i],NULL);
	}
	for(int i=0; i<MAX_REQUEST_RUN; i++)
	{
		requestRun[i]=0;
		requestSocket[i]=0;
		pthread_mutex_init(&requestRunMutex[i],NULL);
	}

	requestAuditThreadId.id=0;
	requestAuditThreadId.created = false;
	pthread_mutex_init(&requestAuditThreadId.id_mutex,NULL);
	pthread_mutex_init(&requestAuditRunningMutex,NULL);
	stopRequestAuditThread=false;
	requestAuditRunning=false;

	connectionAuditThreadId.id=0;
	connectionAuditThreadId.created = false;
	pthread_mutex_init(&connectionAuditThreadId.id_mutex,NULL);
	pthread_mutex_init(&connectionAuditRunningMutex,NULL);
	stopConnectionAuditThread=false;
	connectionAuditRunning=false;

	connectionSchedulerThreadId.id=0;
	connectionSchedulerThreadId.created = false;
	pthread_mutex_init(&connectionSchedulerThreadId.id_mutex,NULL);
	pthread_mutex_init(&connectionSchedulerRunningMutex,NULL);
	stopConnectionSchedulerThread=false;
	connectionSchedulerRunning=false;

	port = _port;
}

P2PRequestServer::~P2PRequestServer()
{
	if(server)
	{
		delete server;
	}

	for(int i=0; i<MAX_REQUEST_RUN; i++)
	{
		pthread_mutex_lock(&requestRunMutex[i]);
		if(requestRun[i])
		{
			requestRun[i]->stop();
			delete requestRun[i];
			requestRun[i]=0;
		}
		if(requestSocket[i])
		{
			delete requestSocket[i];
			requestSocket[i]=0;
		}
		pthread_mutex_unlock(&requestRunMutex[i]);
	}

	for(int i=0; i<MAX_REQUEST_RUN;i++)
	{
		pthread_mutex_destroy(&requestRunMutex[i]);
	}


	for(int i=0; i<MAX_P2P_CONNECTIONS; i++)
	{
		pthread_mutex_lock(&connectionsMutex[i]);
		if(connections[i])
		{
			connections[i]->stop();
			delete connections[i];
			connections[i]=0;
		}
		pthread_mutex_unlock(&connectionsMutex[i]);
	}

	for(int i=0; i<MAX_P2P_CONNECTIONS;i++)
	{
		pthread_mutex_destroy(&connectionsMutex[i]);
	}


	pthread_mutex_destroy(&requestAuditThreadId.id_mutex);
	pthread_mutex_destroy(&connectionAuditThreadId.id_mutex);

	pthread_mutex_destroy(&requestAuditRunningMutex);
	pthread_mutex_destroy(&connectionAuditRunningMutex);
}

bool P2PRequestServer::start()
{
	unsigned int i=0,j=0;
	bool timeoverflow;
	try
	{
		if(!startRequestAudit())
		{
			return false;
		}

		if(!startConnectionAudit())
		{
			return false;
		}
		
		if(!startConnectionScheduler())
		{
			return false;
		}
		
		server = new ServerSocket(port);
		while(1)
		{
			while(1)
			{
				j=0;
				for(i=0;i<MAX_REQUEST_RUN;i++)
				{
					pthread_mutex_lock(&requestRunMutex[i]);
					if(requestRun[i]==0 && requestSocket[i]==0)
					{
						j=1;
						pthread_mutex_unlock(&requestRunMutex[i]);
						break;
					}
					pthread_mutex_unlock(&requestRunMutex[i]);
				}
				if(j)
				{
					break;
				}
				usleep(500000);
			}

			requestSocket[i] = server->startListening(0,&timeoverflow); //It Blocks

			if(!requestSocket[i])
			{
				cout << "Server returned zero." << endl;
				continue;
			}

			pthread_mutex_lock(&requestRunMutex[i]);
			requestRun[i] = new P2PRequestRun(requestSocket[i], this);
			if(!requestRun[i]->start())
			{  //Failed
				cout << "Request " << i << " failed to start." << endl;
				requestRun[i]->stop();
				if(requestSocket[i])
				{
					delete requestSocket[i];
					requestSocket[i]=0;
				}
				if(requestRun[i])
				{
					delete requestRun[i];
					requestRun[i]=0;
				}
			}
			pthread_mutex_unlock(&requestRunMutex[i]);
		}
	}
	catch (SocketException& e)
	{
		cout << "Exception was caught:" << e.description() << "\nExiting.\n";
	}
	return false;
}
void P2PRequestServer::stop()
{
	stopRequestAudit();
	stopConnectionAudit();
	stopConnectionScheduler();
}
bool P2PRequestServer::isUp()
{
	//TODO: Implement
	return true;	
}

bool P2PRequestServer::connectCamera(unsigned long long _cameraId, unsigned char* _p2pId, unsigned int _p2pIdSize, unsigned char* _user, unsigned short _userSize, unsigned char* _password, unsigned short _passwordSize)
{
	if(_cameraId==0)
	{
		return true;
	}
	if(isCameraConnected(_cameraId))
	{
		return true;
	}

	unsigned int i=0;
	for(i=0; i<MAX_P2P_CONNECTIONS; i++)
	{
		pthread_mutex_lock(&connectionsMutex[i]);
		if(connections[i]==0)
		{
			connections[i] = new P2PConnection(_cameraId, _p2pId, _p2pIdSize, _user, _userSize, _password, _passwordSize);
			if(connections[i])
			{
				if(connections[i]->start())
				{
					pthread_mutex_unlock(&connectionsMutex[i]);
					std::cout << "Connection " << i << " started" << std::endl;
					return true;
				}
				else
				{
					connections[i]->stop();
					delete connections[i];
					connections[i]=0;
					pthread_mutex_unlock(&connectionsMutex[i]);
					return false;
				}
			}
			else
			{
				pthread_mutex_unlock(&connectionsMutex[i]);
				return false;
			}

		}
		pthread_mutex_unlock(&connectionsMutex[i]);
	}

	return false;
}

bool P2PRequestServer::isCameraConnected(unsigned long long cameraId)
{
	if(cameraId==0)
	{
		return true;
	}

	for(unsigned int i=0; i<MAX_P2P_CONNECTIONS; i++)
	{
		pthread_mutex_lock(&connectionsMutex[i]);
		if(connections[i]!=0)
		{
			if(connections[i]->getCameraId() == cameraId)
			{
				pthread_mutex_unlock(&connectionsMutex[i]);
				return true;
			} 
		}
		pthread_mutex_unlock(&connectionsMutex[i]);
	}
	return false;
}

bool P2PRequestServer::disconnectCamera(unsigned long long cameraId)
{
	if(cameraId==0)
	{
		return false;
	}

	for(unsigned int i=0; i<MAX_P2P_CONNECTIONS; i++)
	{
		pthread_mutex_lock(&connectionsMutex[i]);
		if(connections[i])
		{
			if(connections[i]->getCameraId() == cameraId)
			{
				connections[i]->stop();
				
				delete connections[i];
				connections[i]=0;
				pthread_mutex_unlock(&connectionsMutex[i]);
				return true;

			} 
		}
		pthread_mutex_unlock(&connectionsMutex[i]);
	}
	return false;
}

unsigned char * P2PRequestServer::getSnapshot(unsigned long long cameraId, unsigned long long *out_size)
{
	if(cameraId==0)
	{
		*out_size=0;
		return 0;
	}

	unsigned char * snap=0;
	unsigned long int snapSize=0;

	for(unsigned int i=0; i<MAX_P2P_CONNECTIONS; i++)
	{
		pthread_mutex_lock(&connectionsMutex[i]);
		if(connections[i])
		{
			if(connections[i]->getCameraId() == cameraId)
			{
				snap = connections[i]->getLastSnapshot(&snapSize);
				*out_size = snapSize;
				pthread_mutex_unlock(&connectionsMutex[i]);
//				std::cout << "getSnapshot connections " << i << " Id " << connections[i]->getCameraId()  << " RequestedId " << cameraId << std::endl;
				return snap;
			} 
		}
		pthread_mutex_unlock(&connectionsMutex[i]);
	}
	*out_size = snapSize;
	return snap;
}

P2PConnectionState P2PRequestServer::getCameraState(unsigned long long cameraId)
{
	if(cameraId==0)
	{
		return State_Disconnected;
	}

	P2PConnectionState ret;
	for(unsigned int i=0; i<MAX_P2P_CONNECTIONS; i++)
	{
		pthread_mutex_lock(&connectionsMutex[i]);
		if(connections[i])
		{
			if(connections[i]->getCameraId() == cameraId)
			{
				ret = connections[i]->getConnectionState();
				pthread_mutex_unlock(&connectionsMutex[i]);
				return ret;
			} 
		}
		pthread_mutex_unlock(&connectionsMutex[i]);
	}
	return State_Disconnected;
}

bool P2PRequestServer::startRequestAudit()
{
	pthread_mutex_lock(&requestAuditThreadId.id_mutex);
	bool ret = (pthread_create(&requestAuditThreadId.id, NULL, runRequestAudit, this) == 0);

	if(ret)
	{
		requestAuditThreadId.created=true;
	}

	pthread_mutex_unlock(&requestAuditThreadId.id_mutex);
	return ret;
}
void P2PRequestServer::stopRequestAudit()
{
	stopRequestAuditThread=true;

	pthread_mutex_lock(&requestAuditThreadId.id_mutex);
	if(requestAuditThreadId.created)
	{
		pthread_join(requestAuditThreadId.id,NULL);
		requestAuditThreadId.created = false;
	}

	pthread_mutex_unlock(&requestAuditThreadId.id_mutex);
}
void* P2PRequestServer::runRequestAudit(void* context)
{
	return ((P2PRequestServer*)context)->requestAuditThread();
}
void* P2PRequestServer::requestAuditThread()
{
	unsigned int i=0;
	pthread_mutex_lock(&requestAuditRunningMutex);
	requestAuditRunning=true;
	pthread_mutex_unlock(&requestAuditRunningMutex);

	while(1)
	{
		sleep(1);
		if(stopRequestAuditThread)
		{
			break;
		}
		
		for(i=0;i<MAX_REQUEST_RUN;i++)
		{
			pthread_mutex_lock(&requestRunMutex[i]);
			if(requestRun[i])
			{
				if(!requestRun[i]->isRunning() || !requestSocket[i])
				{
					requestRun[i]->stop();
					delete requestRun[i];
					requestRun[i]=0;
					if(requestSocket[i])
					{
						delete requestSocket[i];
						requestSocket[i]=0;
					}
					std::cout << "Deleted Request " << i << std::endl;
				}
			}
			else
			{
				if(requestSocket[i])
				{
					delete requestSocket[i];
					requestSocket[i]=0;
				}
			}
			pthread_mutex_unlock(&requestRunMutex[i]);
		}
	}
	pthread_mutex_lock(&requestAuditRunningMutex);
	requestAuditRunning=false;
	pthread_mutex_unlock(&requestAuditRunningMutex);

	return 0;
}

bool P2PRequestServer::startConnectionAudit()
{
	pthread_mutex_lock(&connectionAuditThreadId.id_mutex);
	bool ret = (pthread_create(&connectionAuditThreadId.id, NULL, runConnectionAudit, this) == 0);

	if(ret)
	{
		connectionAuditThreadId.created=true;
	}

	pthread_mutex_unlock(&connectionAuditThreadId.id_mutex);
	return ret;
}
void P2PRequestServer::stopConnectionAudit()
{
	stopConnectionAuditThread=true;

	pthread_mutex_lock(&connectionAuditThreadId.id_mutex);
	if(connectionAuditThreadId.created)
	{
		pthread_join(connectionAuditThreadId.id,NULL);
		connectionAuditThreadId.created = false;
	}

	pthread_mutex_unlock(&connectionAuditThreadId.id_mutex);
}
void* P2PRequestServer::runConnectionAudit(void* context)
{
	return ((P2PRequestServer*)context)->connectionAuditThread();
}
void* P2PRequestServer::connectionAuditThread()
{
	unsigned int i=0;
	pthread_mutex_lock(&connectionAuditRunningMutex);
	connectionAuditRunning=true;
	pthread_mutex_unlock(&connectionAuditRunningMutex);

	while(1)
	{
		sleep(1);
		if(stopConnectionAuditThread)
		{
			break;
		}
		
		for(i=0;i<MAX_P2P_CONNECTIONS;i++)
		{
			pthread_mutex_lock(&connectionsMutex[i]);
			if(connections[i])
			{
				if(!connections[i]->isRunning())
				{
					connections[i]->stop();

					connectionsRet[i]=connections[i]->getReturn();
					delete connections[i];
					connections[i]=0;
					std::cout << "Deleted Connection " << i << " ret " << connectionsRet[i] << std::endl;
				}
			}
			pthread_mutex_unlock(&connectionsMutex[i]);
		}
	}
	pthread_mutex_lock(&connectionAuditRunningMutex);
	connectionAuditRunning=false;
	pthread_mutex_unlock(&connectionAuditRunningMutex);

	return 0;
}

bool P2PRequestServer::startConnectionScheduler()
{
	pthread_mutex_lock(&connectionSchedulerThreadId.id_mutex);
	bool ret = (pthread_create(&connectionSchedulerThreadId.id, NULL, runConnectionScheduler, this) == 0);

	if(ret)
	{
		connectionSchedulerThreadId.created=true;
	}

	pthread_mutex_unlock(&connectionSchedulerThreadId.id_mutex);
	return ret;
}
void P2PRequestServer::stopConnectionScheduler()
{
	stopConnectionSchedulerThread=true;

	pthread_mutex_lock(&connectionSchedulerThreadId.id_mutex);
	if(connectionSchedulerThreadId.created)
	{
		pthread_join(connectionSchedulerThreadId.id,NULL);
		connectionSchedulerThreadId.created = false;
	}

	pthread_mutex_unlock(&connectionSchedulerThreadId.id_mutex);
}
void* P2PRequestServer::runConnectionScheduler(void* context)
{
	return ((P2PRequestServer*)context)->connectionSchedulerThread();
}
void* P2PRequestServer::connectionSchedulerThread()
{
	int i=0,j=0;
	pthread_mutex_lock(&connectionSchedulerRunningMutex);
	connectionSchedulerRunning=true;
	pthread_mutex_unlock(&connectionSchedulerRunningMutex);

	while(1)
	{
		usleep(100000);
		if(stopConnectionSchedulerThread)
		{
			break;
		}
		
		for(i=0;i<MAX_P2P_CONNECTIONS;i++)
		{
			pthread_mutex_lock(&connectionsMutex[i]);
			if(connections[i])
			{
				if(connections[i]->isRunning() && connections[i]->getConnectionState()==State_Waiting)
				{
					pthread_mutex_unlock(&connectionsMutex[i]);
					for(j=0;j<MAX_P2P_CONNECTIONS;j++)
					{
						if(i==j)
						{
							continue;
						}
						while(1)
						{
							pthread_mutex_lock(&connectionsMutex[j]);
							if(connections[j])
							{
								if(connections[j]->isRunning())
								{
									if(connections[j]->getConnectionState()==State_Connecting || connections[j]->getConnectionState()==State_OpenningChannel || connections[j]->getConnectionState()==State_AskingCamera)
									{
										pthread_mutex_unlock(&connectionsMutex[j]);
										usleep(30000);
										continue;
									}
									else
									{
										pthread_mutex_unlock(&connectionsMutex[j]);
										break;
									}
								}
								else
								{
									pthread_mutex_unlock(&connectionsMutex[j]);
									break;
								}
							}
							else
							{
								pthread_mutex_unlock(&connectionsMutex[j]);
								break;
							}
						}
					}
					pthread_mutex_lock(&connectionsMutex[i]);
					connections[i]->performConnection();
					pthread_mutex_unlock(&connectionsMutex[i]);
					std::cout << "Connection " << i << " released" << std::endl;
					usleep(100000);
				}
				else
				{
					pthread_mutex_unlock(&connectionsMutex[i]);
				}
			}
			else
			{
				pthread_mutex_unlock(&connectionsMutex[i]);
			}
		}
	}
	pthread_mutex_lock(&connectionSchedulerRunningMutex);
	connectionSchedulerRunning=false;
	pthread_mutex_unlock(&connectionSchedulerRunningMutex);

	return 0;
}

P2PRequestRun::P2PRequestRun(DataSocket * _recvSocket, P2PRequestServer * _caller)
{
	recvSocket = _recvSocket;
	caller = _caller;
	
	requestRunThreadId.id=0;
	requestRunThreadId.created = false;
	pthread_mutex_init(&requestRunThreadId.id_mutex,NULL);
	stopRequestRunThread=false;
	requestRunRunning=false;
}

P2PRequestRun::~P2PRequestRun()
{
	pthread_mutex_destroy(&requestRunThreadId.id_mutex);
}

bool P2PRequestRun::start()
{
	return startRequestRun();
}

void P2PRequestRun::stop()
{
	stopRequestRun();
}

bool P2PRequestRun::isRunning()
{
	pthread_mutex_lock(&requestRunRunningMutex);
	if(requestRunRunning)
	{
		pthread_mutex_unlock(&requestRunRunningMutex);
		return true;
	}
	else
	{
		pthread_mutex_unlock(&requestRunRunningMutex);
		return false;
	}
}

bool P2PRequestRun::startRequestRun()
{
	pthread_mutex_lock(&requestRunThreadId.id_mutex);
	bool ret = (pthread_create(&requestRunThreadId.id, NULL, runRequestRun, this) == 0);

	if(ret)
	{
		requestRunThreadId.created=true;
	}

	pthread_mutex_unlock(&requestRunThreadId.id_mutex);
	return ret;
}

void P2PRequestRun::stopRequestRun()
{
	stopRequestRunThread=true;

	pthread_mutex_lock(&requestRunThreadId.id_mutex);
	if(requestRunThreadId.created)
	{
		pthread_join(requestRunThreadId.id,NULL);
		requestRunThreadId.created = false;
	}

	pthread_mutex_unlock(&requestRunThreadId.id_mutex);
}

void* P2PRequestRun::runRequestRun(void* context)
{
	return ((P2PRequestRun*)context)->requestRunThread();
}

void* P2PRequestRun::requestRunThread()
{
	pthread_mutex_lock(&requestRunRunningMutex);
	requestRunRunning=true;
	pthread_mutex_unlock(&requestRunRunningMutex);
	P2PMessage * incomingRequest;
	P2PMessage * answer;
	int recvSize=0;
	int recvErrno;
	unsigned char buffer[500];
	unsigned char packet_type=0;

	unsigned char* snapshot;
	unsigned long long snapshotSize;

	while(1)
	{
		if(stopRequestRunThread)
		{
			break;
		}
		
		while(1)
		{
			if(stopRequestRunThread)
			{
				break;
			}

			recvSize=0;
			recvSize = recvSocket->receiveData(buffer, 250, 500, &recvErrno);
			if(recvSize==-1 && recvErrno!=EWOULDBLOCK && recvErrno!=EAGAIN)
			{
				//socket error
				stopRequestRunThread=true;
				break;
			}
			else if(recvSize==0)
			{
				//Connection Loss
				stopRequestRunThread=true;
				break;
			}
			else if(recvSize>0)
			{
				if(recvSize>=9)
				{
					packet_type = P2PMessage::getTypeFromRawInfo(buffer,recvSize);
					if(packet_type==P2P_CONNECT_CAMERA)
					{
						incomingRequest = new ConnectRequest();
						incomingRequest->unserialize(buffer,(unsigned int)recvSize);

						if(caller->isCameraConnected(incomingRequest->getCameraId()))
						{
							{
								answer = new AlreadyRequestedAnswer(incomingRequest->getCameraId());
								unsigned int answerSize;
								unsigned char *answerBytes = answer->serialize(&answerSize);
								bool timeover;
								recvSocket->sendData(answerBytes, answerSize, 0, &timeover);

								if(answerBytes)
								{
									delete[] answerBytes;
									answerBytes=0;
								}
								if(answer)
								{
									delete answer;
									answer=0;
								}
							}

							if(incomingRequest)
							{
								delete incomingRequest;
								incomingRequest=0;
							}
							break;
						}

						unsigned int p2pIdsz;
						unsigned char * p2pId = ((ConnectRequest*)incomingRequest)->getP2PId(&p2pIdsz);

						unsigned short userloginsz;
						unsigned char * userlogin = ((ConnectRequest*)incomingRequest)->getUserLogin(&userloginsz);
						unsigned short passwordsz;
						unsigned char * password = ((ConnectRequest*)incomingRequest)->getPasswordLogin(&passwordsz);
						unsigned long long cmrId= incomingRequest->getCameraId();

						if(caller->connectCamera(cmrId,p2pId,p2pIdsz,userlogin,userloginsz,password,passwordsz))
						{
							{
								answer = new CameraConnectingAnswer(incomingRequest->getCameraId());
								unsigned int answerSize;
								unsigned char *answerBytes = answer->serialize(&answerSize);
								bool timeover;
								recvSocket->sendData(answerBytes, answerSize, 0, &timeover);
								if(answerBytes)
								{
									delete[] answerBytes;
									answerBytes=0;
								}
								if(answer)
								{
									delete answer;
									answer=0;
								}
							}
						}
						else
						{
							{
								answer = new ErrorAnswer(incomingRequest->getCameraId());
								unsigned int answerSize;
								unsigned char *answerBytes = answer->serialize(&answerSize);
								bool timeover;
								recvSocket->sendData(answerBytes, answerSize, 0, &timeover);
								if(answerBytes)
								{
									delete[] answerBytes;
									answerBytes=0;
								}
								if(answer)
								{
									delete answer;
									answer=0;
								}
							}
						}

						if(incomingRequest)
						{
							delete incomingRequest;
							incomingRequest=0;
						}
						break;
					}
					else if(packet_type==P2P_DISCONNECT_CAMERA)
					{
						incomingRequest = new DisconnectRequest();
						incomingRequest->unserialize(buffer,(unsigned int)recvSize);

						caller->disconnectCamera(incomingRequest->getCameraId());
						
						if(incomingRequest)
						{
							delete incomingRequest;
							incomingRequest=0;
						}
						stopRequestRunThread=true;
						std::cout << "Disconnecting camera" << std::endl;
						break;
					}
					else if(packet_type==P2P_GET_SNAPSHOT)
					{
						incomingRequest = new GetSnapshotRequest();
						incomingRequest->unserialize(buffer,(unsigned int)recvSize);
						if(!caller->isCameraConnected(incomingRequest->getCameraId()))
						{
							{
								answer = new CameraOfflineAnswer(incomingRequest->getCameraId());
								unsigned int answerSize;
								unsigned char *answerBytes = answer->serialize(&answerSize);
								bool timeover;
								recvSocket->sendData(answerBytes, answerSize, 0, &timeover);

								delete[] answerBytes;
								answerBytes=0;
								delete answer;
								answer=0;
							}
							
							if(incomingRequest)
							{
								delete incomingRequest;
								incomingRequest=0;
							}
							break;
						}

						snapshot = caller->getSnapshot(incomingRequest->getCameraId(),&snapshotSize);
						if(!snapshot || snapshotSize==0)
						{
							if(snapshot)
							{
								delete[] snapshot;
								snapshot=0;
							}
							P2PConnectionState state = caller->getCameraState(incomingRequest->getCameraId());
							if(state==State_Disconnected)
							{

								{
									answer = new CameraOfflineAnswer(incomingRequest->getCameraId());
									unsigned int answerSize;
									unsigned char *answerBytes = answer->serialize(&answerSize);
									bool timeover;
									recvSocket->sendData(answerBytes, answerSize, 0, &timeover);

									delete[] answerBytes;
									answerBytes=0;
									delete answer;
									answer=0;
								}
								if(incomingRequest)
								{
									delete incomingRequest;
									incomingRequest=0;
								}
								break;
							}
							else
							{
								{
									answer = new CameraConnectingAnswer(incomingRequest->getCameraId());
									unsigned int answerSize;
									unsigned char *answerBytes = answer->serialize(&answerSize);
									bool timeover;
									recvSocket->sendData(answerBytes, answerSize, 0, &timeover);

									delete[] answerBytes;
									answerBytes=0;
									delete answer;
									answer=0;
								}
								if(incomingRequest)
								{
									delete incomingRequest;
									incomingRequest=0;
								}
								break;
							}
						}
						else
						{
							{
								answer = new SnapshotAnswer(incomingRequest->getCameraId(),snapshotSize,snapshot);
								unsigned int answerSize;
								unsigned char *answerBytes = answer->serialize(&answerSize);
								bool timeover;
								recvSocket->sendData(answerBytes, answerSize, 0, &timeover);

								delete[] snapshot;
								snapshot=0;
								delete[] answerBytes;
								answerBytes=0;
								delete answer;
								answer=0;
							}
							if(incomingRequest)
							{
								delete incomingRequest;
								incomingRequest=0;
							}
							break;
						}

						/*****Output the snapshot request to recvSocket*************/

						break;
					}
					else
					{
						break;
					}
				}
				else
				{
					//TODO receive again and sum up the recvSize
				}
			}
			else
			{
				//No Data received
			}
		}

		if(stopRequestRunThread)
		{
			break;
		}
	}
	pthread_mutex_lock(&requestRunRunningMutex);
	requestRunRunning=false;
	pthread_mutex_unlock(&requestRunRunningMutex);

	return 0;
}