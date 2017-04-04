#ifndef _NOTIFICATION_SENDER_H_
#define _NOTIFICATION_SENDER_H_

#include "SnitchNotification.h"
#include "Thread.h"
#include <queue>

extern pthread_mutex_t queueMutex;

class NotificationSender
{
public:
	NotificationSender(queue<SnitchNotification*>** _queue);
	~NotificationSender();

	bool run();
	void stop();
	bool isUp();

private:
	PthreadId senderThreadId;
	pthread_mutex_t senderRunningMutex;
	volatile bool stopSenderThread;
	volatile bool senderRunning;
	bool startSender();
	void stopSender();
	static void* runSender(void* context);
	void* senderThread();

	queue<SnitchNotification*>* pendingQueue; //It needs to be thread-safe (Shared Resource)
};

#endif