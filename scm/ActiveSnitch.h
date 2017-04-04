#ifndef _ACTIVE_SNITCH_H_
#define _ACTIVE_SNITCH_H_

#include "Socket.h"
#include "SCMRouter.h"
#include "Thread.h"

typedef enum 
{
	GettingSerialNumber,
	InsertingDb,
	Active,
	DeletingDb,
}ActiveSnitchStatus;

class ActiveSnitch
{
public:
	ActiveSnitch(std::string _filePath, bool _routerSendBlock);
	~ActiveSnitch();

	bool startSnitch(DataSocket* snitch_sock, unsigned int p);
	void stopSnitch();
	unsigned long long getSerialNumber();
	bool isLinkUp();

private:
	unsigned int port;				//-----------------
	unsigned long long snitchSerialNum;	//--Into Database--

	DataSocket * snitchSocket;
	ServerSocket * apacheListenSocket;

	pthread_mutex_t routerMutex;
	SCMRouter * mainRouter;

	PthreadId lnkAuditThreadId;

	bool routerSendBlock;
	bool routeEnable;
	bool linkUp;
	bool stopAudit;
	pthread_mutex_t stopAuditMutex;

	std::string filePath;

	bool deleteOnDestructor;

	bool startLinkAudit();
	bool stopLinkAudit();
	static void* runLinkAudit(void* context);
	void* linkAuditThread();

	void insertDb();
	void deleteDb();
	unsigned long long getSerialNum();
};

#endif