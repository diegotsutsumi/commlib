#ifndef _CLIENTCM_H_
#define _CLIENTCM_H_

#include "Socket.h"
#include "SCMRouter.h"
#include "Thread.h"
#include <string>
#include "AppType.h"

class ClientSCM
{
public:
	ClientSCM(AppType _type, std::string _appIp, unsigned short _appPort);
	~ClientSCM();

	bool runClientSCM();

private:
	AppType type;
	std::string appIp;
	unsigned short appPort;
	ClientSocket * serverLinkSocket;

	unsigned long long serialNumber;

	SCMRouter * mainRouter;

	bool stopHB;
	pthread_mutex_t stopHBMutex;
	PthreadId hBThreadId;

	bool startHeartBeat();
	bool stopHeartBeat();
	static void* runHeartBeat(void* context);
	void* heartBeatThread();
};

#endif