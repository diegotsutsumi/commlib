#ifndef _SERVERSCM_H_
#define _SERVERSCM_H_

#include "Socket.h"
#include "ActiveSnitch.h"
#include "Thread.h"
#include "AppType.h"

#define MAX_SNITCHES 500

class ServerSCM
{
public:
	ServerSCM(AppType _type);
	~ServerSCM();
	bool startServerSCM();
	bool stopServerSCM();
	
private:
	ServerSocket * server;
	DataSocket * linkSockets[MAX_SNITCHES];
	ActiveSnitch * activeSnitches[MAX_SNITCHES];
	pthread_mutex_t snitchMutexes[MAX_SNITCHES];
	AppType type;
	std::string filePath;
	unsigned short serverPort;
	unsigned short baseClientPort;

	PthreadId snAuditThread;
	bool stopServer;
	pthread_mutex_t stopMutex;

	bool startSnitchAudit();
	bool stopSnitchAudit();
	static void* runSnitchAudit(void* context);
	void* snitchAuditThread();

};

#endif