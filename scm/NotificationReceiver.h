#ifndef _NOTIFICATION_RECEIVER_H_
#define _NOTIFICATION_RECEIVER_H_

#include "SnitchNotification.h"
#include "Socket.h"
#include "Thread.h"
#include <queue>
#define MAX_RECEIVER_SOCKETS 10
using std::queue;

extern pthread_mutex_t queueMutex;


class NotificationReceiver
{
public:
	NotificationReceiver(DataSocket* _recvSocket, queue<SnitchNotification*>** _queue);
	~NotificationReceiver();

	bool run();
	void stop();
	bool isRunning();

private:
	PthreadId receiverThreadId;
	pthread_mutex_t receiverRunningMutex;
	volatile bool stopReceiverThread;
	volatile bool receiverRunning;
	bool startReceiver();
	void stopReceiver();
	static void* runReceiver(void* context);
	void* receiverThread();

	DataSocket* recvSocket;
	queue<SnitchNotification*>* pendingQueue;

};


class NotificationListener // Receives Notification from Zoneminder and queue them
{
public:
	NotificationListener(unsigned int _port, queue<SnitchNotification*>** _queue);
	~NotificationListener();

	unsigned int getPort();
	bool run();
	void stop();
	bool isUp();

private:
	PthreadId listenerThreadId;
	pthread_mutex_t listenerRunningMutex;
	volatile bool stopListenerThread;
	volatile bool listenerRunning;
	bool startListener();
	void stopListener();
	static void* runListener(void* context);
	void* listenerThread();

	PthreadId receiverAuditThreadId;
	pthread_mutex_t receiverAuditRunningMutex;
	volatile bool stopReceiverAuditThread;
	volatile bool receiverAuditRunning;
	bool startReceiverAudit();
	void stopReceiverAudit();
	static void* runReceiverAudit(void* context);
	void* receiverAuditThread();

	queue<SnitchNotification*>* pendingQueue; //It needs to be thread-safe (Shared Resource)
	ServerSocket *listener;
	pthread_mutex_t receiversMutex[MAX_RECEIVER_SOCKETS];
	NotificationReceiver * receivers[MAX_RECEIVER_SOCKETS];
	unsigned int port;
};

#endif