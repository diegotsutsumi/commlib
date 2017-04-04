#include "NotificationReceiver.h"
#include "Exception.h"
#include <iostream>
#include "SnitchNotifier.h"

NotificationListener::NotificationListener(unsigned int _port, queue<SnitchNotification*>** _queue)
{
	port = _port;
	pthread_mutex_lock(&queueMutex);
	pendingQueue = *_queue;
	pthread_mutex_unlock(&queueMutex);
	listener = new ServerSocket(port);

	listenerThreadId.id=0;
	listenerThreadId.created=false;
	pthread_mutex_init(&listenerThreadId.id_mutex,NULL);
	pthread_mutex_init(&listenerRunningMutex,NULL);
	listenerRunning=false;
	stopListenerThread=false;

	receiverAuditThreadId.id=0;
	receiverAuditThreadId.created=false;
	pthread_mutex_init(&receiverAuditThreadId.id_mutex,NULL);
	pthread_mutex_init(&receiverAuditRunningMutex,NULL);
	receiverAuditRunning=false;
	stopReceiverAuditThread=false;

	for(int i=0;i<MAX_RECEIVER_SOCKETS;i++)
	{
		pthread_mutex_init(&receiversMutex[i],NULL);
		receivers[i]=0;
	}
}

NotificationListener::~NotificationListener()
{
	for(int i=0;i<MAX_RECEIVER_SOCKETS;i++)
	{
		pthread_mutex_destroy(&receiversMutex[i]);
		receivers[i]=0;
	}
	delete listener;
	pthread_mutex_destroy(&listenerThreadId.id_mutex);
	pthread_mutex_destroy(&listenerRunningMutex);
	pthread_mutex_destroy(&receiverAuditThreadId.id_mutex);
	pthread_mutex_destroy(&receiverAuditRunningMutex);
}

unsigned int NotificationListener::getPort()
{
	return port;
}

bool NotificationListener::run()
{
	bool ret;
	ret = startReceiverAudit();
	return startListener() && ret;
}

void NotificationListener::stop()
{
	stopReceiverAudit();
	stopReceiverAudit();
}

bool NotificationListener::isUp()
{
	bool ret;
	pthread_mutex_lock(&listenerRunningMutex);
	ret=listenerRunning;
	pthread_mutex_unlock(&listenerRunningMutex);
	return ret;
}


bool NotificationListener::startListener()
{
	stopListenerThread=false;

	pthread_mutex_lock(&listenerThreadId.id_mutex);
	bool ret = (pthread_create(&listenerThreadId.id, NULL, runListener, this) == 0);

	if(ret)
	{
		listenerThreadId.created=true;
	}

	pthread_mutex_unlock(&listenerThreadId.id_mutex);

	std::cout << "Listener Started" << std::endl;
	return ret;
}

void NotificationListener::stopListener()
{
	stopListenerThread=true;

	pthread_mutex_lock(&listenerThreadId.id_mutex);
	if(listenerThreadId.created)
	{
		pthread_join(listenerThreadId.id,NULL);
		listenerThreadId.created = false;
	}

	pthread_mutex_unlock(&listenerThreadId.id_mutex);
	std::cout << "Listener Stopped" << std::endl;
}

void* NotificationListener::runListener(void* context)
{
	return ((NotificationListener*)context)->listenerThread();
}

void* NotificationListener::listenerThread()
{
	unsigned int i=0;
	unsigned int j=0;
	bool timeoverflow=false;
	bool stopFlag=false;
	DataSocket* newSock=0;

	pthread_mutex_lock(&listenerRunningMutex);
	listenerRunning=true;
	pthread_mutex_unlock(&listenerRunningMutex);

	try
	{
		while(1)
		{
			if(stopListenerThread)
			{
				break;
			}

			while(1)
			{
				if(stopListenerThread)
				{
					stopFlag=true;
					break;
				}
				j=0;
				for(i=0;i<MAX_RECEIVER_SOCKETS;i++)
				{
					pthread_mutex_lock(&receiversMutex[i]);
					if(receivers[i]==0)
					{
						pthread_mutex_unlock(&receiversMutex[i]);
						j=1;
						break;
					}
					else
					{
						pthread_mutex_unlock(&receiversMutex[i]);
					}
				}
				if(j)
				{
					break;
				}
				usleep(500000);
			}
			if(stopFlag)
			{
				break;
			}
			newSock = 0;
			newSock = listener->startListening(500,&timeoverflow);
			if(timeoverflow)
			{
				newSock=0;
				continue;
			}
			if(!newSock) //Error
			{
				newSock=0;
				continue;
			}

			receivers[i] = new NotificationReceiver(newSock,&pendingQueue);
			receivers[i]->run(); //Audit deletes it
			std::cout << "Receiver " << i << "Created" << std::endl;
		}
	}
	catch(Exception &e)
	{
		std::cout << "Exception caught: " << e.description() <<std::endl;
	}

	/**STOP ALL RECEIVERS

	/**** DELETE EVERY BYTE DYNAMICALLY ALLOCATED INSIDE THE WHILE KEYWORD ****/

	pthread_mutex_lock(&listenerRunningMutex);
	listenerRunning=false;
	pthread_mutex_unlock(&listenerRunningMutex);
	return 0;
}

bool NotificationListener::startReceiverAudit()
{
	stopReceiverAuditThread=false;

	pthread_mutex_lock(&receiverAuditThreadId.id_mutex);
	bool ret = (pthread_create(&receiverAuditThreadId.id, NULL, runReceiverAudit, this) == 0);

	if(ret)
	{
		receiverAuditThreadId.created=true;
	}

	pthread_mutex_unlock(&receiverAuditThreadId.id_mutex);
	std::cout << "Receiver Audit Started" << std::endl;
	return ret;
}

void NotificationListener::stopReceiverAudit()
{
	stopReceiverAuditThread=true;

	pthread_mutex_lock(&receiverAuditThreadId.id_mutex);
	if(receiverAuditThreadId.created)
	{
		pthread_join(receiverAuditThreadId.id,NULL);
		receiverAuditThreadId.created = false;
	}

	pthread_mutex_unlock(&receiverAuditThreadId.id_mutex);
	std::cout << "Receiver Audit Stopped" << std::endl;
}

void* NotificationListener::runReceiverAudit(void* context)
{
	return ((NotificationListener*)context)->receiverAuditThread();
}
void* NotificationListener::receiverAuditThread()
{
	pthread_mutex_lock(&receiverAuditRunningMutex);
	receiverAuditRunning=true;
	pthread_mutex_unlock(&receiverAuditRunningMutex);
	unsigned int timeCounters[MAX_RECEIVER_SOCKETS];

	for(int i=0; i<MAX_RECEIVER_SOCKETS; i++)
	{
		timeCounters[i]=0;
	}

	try
	{
		while(1)
		{
			sleep(1);
			if(stopReceiverAuditThread)
			{
				break;
			}

			for(int i=0;i<MAX_RECEIVER_SOCKETS;i++)
			{
				pthread_mutex_lock(&receiversMutex[i]);
				if(receivers[i])
				{
					std::cout << "Receiver " << i << " Not zero" << std::endl;
					if(!receivers[i]->isRunning())
					{
						receivers[i]->stop();
						delete receivers[i];
						receivers[i]=0;
						std::cout << "Receiver " << i << " Deleted by finished run" << std::endl;
					}
					else
					{
						if(timeCounters[i]>=10)
						{
							receivers[i]->stop();
							delete receivers[i];
							receivers[i]=0;
							std::cout << "Receiver " << i << " Deleted by timeout" << std::endl;
						}
						else
						{
							timeCounters[i]++;
						}
					}
				}
				pthread_mutex_unlock(&receiversMutex[i]);
			}
		}
	}
	catch(Exception &e)
	{
		std::cout << "Exception caught: " << e.description() <<std::endl;
	}

	for(int i=0;i<MAX_RECEIVER_SOCKETS;i++)
	{
		pthread_mutex_lock(&receiversMutex[i]);
		if(receivers[i])
		{
			delete receivers[i];
			receivers[i]=0;
		}
		pthread_mutex_unlock(&receiversMutex[i]);
	}

	pthread_mutex_lock(&receiverAuditRunningMutex);
	receiverAuditRunning=false;
	pthread_mutex_unlock(&receiverAuditRunningMutex);
	return 0;
}






NotificationReceiver::NotificationReceiver(DataSocket* _recvSocket, queue<SnitchNotification*>** _queue)
{
	recvSocket = _recvSocket;
	pendingQueue = *_queue;

	receiverThreadId.id=0;
	receiverThreadId.created=false;
	pthread_mutex_init(&receiverThreadId.id_mutex,NULL);
	pthread_mutex_init(&receiverRunningMutex,NULL);
	receiverRunning=false;
	stopReceiverThread=false;
}
NotificationReceiver::~NotificationReceiver()
{
	pthread_mutex_destroy(&receiverThreadId.id_mutex);
	pthread_mutex_destroy(&receiverRunningMutex);
	if(recvSocket)
		delete recvSocket;
}

bool NotificationReceiver::run()
{
	return startReceiver();
}
void NotificationReceiver::stop()
{
	stopReceiver();
}
bool NotificationReceiver::isRunning()
{
	bool ret;
	pthread_mutex_lock(&receiverRunningMutex);
	ret = receiverRunning;
	pthread_mutex_unlock(&receiverRunningMutex);
	return ret;
}

bool NotificationReceiver::startReceiver()
{
	stopReceiverThread=false;

	pthread_mutex_lock(&receiverThreadId.id_mutex);
	bool ret = (pthread_create(&receiverThreadId.id, NULL, runReceiver, this) == 0);

	if(ret)
	{
		receiverThreadId.created=true;
	}

	pthread_mutex_unlock(&receiverThreadId.id_mutex);
	return ret;
}

void NotificationReceiver::stopReceiver()
{
	stopReceiverThread=true;

	pthread_mutex_lock(&receiverThreadId.id_mutex);
	if(receiverThreadId.created)
	{
		pthread_join(receiverThreadId.id,NULL);
		receiverThreadId.created = false;
	}

	pthread_mutex_unlock(&receiverThreadId.id_mutex);
}

void* NotificationReceiver::runReceiver(void* context)
{
	return ((NotificationReceiver*)context)->receiverThread();
}

void* NotificationReceiver::receiverThread()
{
	bool stopFlag=false;
	unsigned char packet_type=0;

	unsigned char buffer[50];
	int recvSize=0;
	int recvErrno;

	SnitchNotification * newNotification;
	memset(buffer, 0, sizeof(buffer));

	pthread_mutex_lock(&receiverRunningMutex);
	receiverRunning=true;
	pthread_mutex_unlock(&receiverRunningMutex);

	try
	{
		while(1)
		{
			if(stopReceiverThread)
			{
				break;
			}
			//Receive Notification Information
			while(1)
			{
				if(stopReceiverThread)
				{
					stopFlag=true;
					break;
				}


				recvSize=0;
				recvSize = recvSocket->receiveData(buffer, 50, 500, &recvErrno);
				if(recvSize==-1 && recvErrno!=EWOULDBLOCK && recvErrno!=EAGAIN)
				{
					//socket error
					stopFlag=true;
					break;
				}
				else if(recvSize==0)
				{
					//Connection Loss
					stopFlag=true;
					break;
				}
				else if(recvSize>0)
				{
					if(recvSize>=9) //2 Types of packet we have for now
					{
						packet_type = SnitchNotification::getTypeFromRawInfo(buffer,recvSize);
						if(packet_type==MOTION_NOTIFICATION_TYPE)
						{
							newNotification = new MotionNotification(); //Should be deleted by the NotificationSender
							newNotification->unserialize(buffer,(unsigned int)recvSize);
							pthread_mutex_lock(&queueMutex);
							pendingQueue->push(newNotification);
							pthread_mutex_unlock(&queueMutex);
							std::cout << "Received Motion Notification " << newNotification->getShellCommand() << std::endl;
							newNotification=0;
							stopFlag=true;
							break;
						}
						else if(packet_type==SIGNALLOSS_NOTIFICATION_TYPE)
						{
							newNotification = new SignalLossNotification(); //Should be deleted by the NotificationSender
							newNotification->unserialize(buffer,(unsigned int)recvSize);
							
							pthread_mutex_lock(&queueMutex);
							pendingQueue->push(newNotification);
							pthread_mutex_unlock(&queueMutex);
							std::cout << "Received Signal Loss Notification " << newNotification->getShellCommand() << std::endl;
							newNotification=0;
							stopFlag=true;
							break;
						}
						else
						{
							stopFlag=true;
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
			if(stopFlag)
			{
				break;
			}

		}
	}
	catch(Exception &e)
	{
		std::cout << "Exception caught: " << e.description() <<std::endl;
	}

	/**** DELETE EVERY BYTE DYNAMICALLY ALLOCATED INSIDE THE WHILE KEYWORD ****/

	pthread_mutex_lock(&receiverRunningMutex);
	receiverRunning=false;
	pthread_mutex_unlock(&receiverRunningMutex);
	return 0;
}