#ifndef _P2PREQUEST_RECEIVER_H_
#define _P2PREQUEST_RECEIVER_H_

#include "P2PConnection.h"
#include "P2PMessage.h"
#include "Socket.h"
#include "Thread.h"

#define MAX_REQUEST_RUN 500
#define MAX_P2P_CONNECTIONS 200

class P2PRequestRun;

class P2PRequestServer //Receives P2P Requests from Zoneminder
{
public:
	P2PRequestServer(unsigned int _port);
	~P2PRequestServer();

	unsigned int getPort();
	bool start();
	void stop();
	bool isUp();

	bool connectCamera(unsigned long long _cameraId, unsigned char* _p2pId, unsigned int _p2pIdSize, unsigned char* _user, unsigned short _userSize, unsigned char* _password, unsigned short _passwordSize);
	bool isCameraConnected(unsigned long long cameraId);
	bool disconnectCamera(unsigned long long cameraId);
	unsigned char * getSnapshot(unsigned long long cameraId, unsigned long long *out_size);
	P2PConnectionState getCameraState(unsigned long long cameraId);

private:

	PthreadId requestAuditThreadId;
	pthread_mutex_t requestAuditRunningMutex;
	volatile bool stopRequestAuditThread;
	volatile bool requestAuditRunning;
	bool startRequestAudit();
	void stopRequestAudit();
	static void* runRequestAudit(void* context);
	void* requestAuditThread();

	PthreadId connectionAuditThreadId;
	pthread_mutex_t connectionAuditRunningMutex;
	volatile bool stopConnectionAuditThread;
	volatile bool connectionAuditRunning;
	bool startConnectionAudit();
	void stopConnectionAudit();
	static void* runConnectionAudit(void* context);
	void* connectionAuditThread();

	PthreadId connectionSchedulerThreadId;
	pthread_mutex_t connectionSchedulerRunningMutex;
	volatile bool stopConnectionSchedulerThread;
	volatile bool connectionSchedulerRunning;
	bool startConnectionScheduler();
	void stopConnectionScheduler();
	static void* runConnectionScheduler(void* context);
	void* connectionSchedulerThread();

	ServerSocket *server;
	pthread_mutex_t requestRunMutex[MAX_REQUEST_RUN];
	P2PRequestRun * requestRun[MAX_REQUEST_RUN];
	DataSocket * requestSocket[MAX_REQUEST_RUN];

	pthread_mutex_t connectionsMutex[MAX_P2P_CONNECTIONS];
	P2PConnection * connections[MAX_P2P_CONNECTIONS];
	P2PReturn connectionsRet[MAX_P2P_CONNECTIONS];
	unsigned int port;
};

class P2PRequestRun
{
public:
	P2PRequestRun(DataSocket* _recvSocket, P2PRequestServer * _caller);
	~P2PRequestRun();

	bool start();
	void stop();
	bool isRunning();

private:
	PthreadId requestRunThreadId;
	pthread_mutex_t requestRunRunningMutex;
	volatile bool stopRequestRunThread;
	volatile bool requestRunRunning;
	bool startRequestRun();
	void stopRequestRun();
	static void* runRequestRun(void* context);
	void* requestRunThread();

	P2PRequestServer * caller;
	DataSocket* recvSocket;
};

#endif