#include "NotificationSender.h"
#include "Exception.h"
#include <unistd.h>
#include <iostream>
#include "SnitchNotifier.h"

NotificationSender::NotificationSender(queue<SnitchNotification*>** _queue)
{
	pthread_mutex_lock(&queueMutex);
	pendingQueue = *_queue;
	pthread_mutex_unlock(&queueMutex);

	senderThreadId.id=0;
	senderThreadId.created=false;
	pthread_mutex_init(&senderThreadId.id_mutex,NULL);
	pthread_mutex_init(&senderRunningMutex,NULL);
	senderRunning=false;
}

NotificationSender::~NotificationSender()
{
	pthread_mutex_destroy(&senderRunningMutex);
}

bool NotificationSender::run()
{
	return startSender();
}

void NotificationSender::stop()
{
	return stopSender();
}

bool NotificationSender::isUp()
{
	bool ret;
	pthread_mutex_lock(&senderRunningMutex);
	ret = senderRunning;
	pthread_mutex_unlock(&senderRunningMutex);
	return ret;
}

bool NotificationSender::startSender() //Waits for apache connection and creates a channel with the Snitch
{
	stopSenderThread=false;

	pthread_mutex_lock(&senderThreadId.id_mutex);
	bool ret = (pthread_create(&senderThreadId.id, NULL, runSender, this) == 0);

	if(ret)
	{
		senderThreadId.created=true;
	}

	pthread_mutex_unlock(&senderThreadId.id_mutex);
	std::cout << "Sender Started" << std::endl;
	return ret;
}

void NotificationSender::stopSender()
{
	stopSenderThread=true;

	pthread_mutex_lock(&senderThreadId.id_mutex);
	if(senderThreadId.created)
	{
		pthread_join(senderThreadId.id,NULL);
		senderThreadId.created = false;
	}

	pthread_mutex_unlock(&senderThreadId.id_mutex);
	std::cout << "Sender Stopped" << std::endl;
}

void* NotificationSender::runSender(void* context)
{
	return ((NotificationSender*)context)->senderThread();
}

void* NotificationSender::senderThread()
{

	pthread_mutex_lock(&senderRunningMutex);
	senderRunning=true;
	pthread_mutex_unlock(&senderRunningMutex);

	SnitchNotification * toSend;
	std::string cmdShell = "";
	int systemRet;

	try
	{
		while(1)
		{
			if(stopSenderThread)
			{
				break;
			}
			
			pthread_mutex_lock(&queueMutex);
			if(pendingQueue->size()>0)
			{
				toSend = pendingQueue->front();
				cmdShell = toSend->getShellCommand();
				systemRet = system(cmdShell.c_str());

				if(!systemRet)
				{
					std::cout << "Sent Notification " << cmdShell << std::endl;
					pendingQueue->pop();
					delete toSend;
					toSend=0;
					cmdShell="";
				}
				else
				{
					std::cout << "Couldn't sent " << cmdShell << std::endl;
					cmdShell="";
					toSend=0;
				}
				pthread_mutex_unlock(&queueMutex);
			}
			else
			{
				pthread_mutex_unlock(&queueMutex);
			}
			usleep(300000);
		}
	}
	catch(Exception &e)
	{
		std::cout << "Exception caught: " << e.description() <<std::endl;
	}

	/**** DELETE EVERY BYTE DYNAMICALLY ALLOCATED INSIDE THE TRY CATCH KEYWORD ****/

	pthread_mutex_lock(&senderRunningMutex);
	senderRunning=false;
	pthread_mutex_unlock(&senderRunningMutex);
	return 0;

}