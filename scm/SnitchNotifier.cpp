#include "SnitchNotifier.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <sys/stat.h>

pthread_mutex_t queueMutex;

SnitchNotifier::SnitchNotifier(unsigned int receiver_port)
{
	pthread_mutex_init(&queueMutex,NULL);
	pthread_mutex_lock(&queueMutex);
	pendingNotifications = new queue<SnitchNotification*>();
	pthread_mutex_unlock(&queueMutex);
	receiver = new NotificationListener(receiver_port, &pendingNotifications);
	sender = new NotificationSender(&pendingNotifications);
}

SnitchNotifier::~SnitchNotifier()
{
	if(receiver)
		delete receiver;
	if(sender)
		delete sender;
	pthread_mutex_lock(&queueMutex);
	if(pendingNotifications)
		delete pendingNotifications;
	pthread_mutex_unlock(&queueMutex);

	pthread_mutex_destroy(&queueMutex);
}

bool SnitchNotifier::run()
{
	if(!receiver->run())
	{
		std::cout << "Receiver False" << std::endl;
		return false;
	}
	if(!sender->run())
	{
		std::cout << "Sender False" << std::endl;
		return false;
	}

	while(1)
	{
		sleep(2);
		if(!receiver->isUp())
		{
			std::cout << "Receiver Down" << std::endl;
			return false;
		}
		if(!sender->isUp())
		{
			std::cout << "Sender Down" << std::endl;
			return false;
		}
	}
}

bool SnitchNotifier::stop()
{
	receiver->stop();
	sender->stop();
	
	return true;
}