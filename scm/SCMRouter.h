#ifndef _SCMROUTER_H_
#define _SCMROUTER_H_

#include "Socket.h"
#include "SCMPacket.h"
#include "Thread.h"

#define CHANNEL_TIMEOUT 15

class SCMHalfChannel : public HalfDuplexChannel
{
public:
	SCMHalfChannel(DataSocket * _in, DataSocket * _out, unsigned int buffer_size, unsigned int size_overhead, unsigned char _chnNumber, SCMProtocol::PeerType _mpeerType, unsigned long long _mypeerId);
	~SCMHalfChannel();
private:
	unsigned char chnNumber;
	unsigned long long myPeerId;
	SCMProtocol::PeerType myPeerType;
	long int processBuffer(unsigned char* buffer, long int _recvSize); //Polymorphism
};


class SCMRouter
{
public:
	SCMRouter(SCMProtocol::PeerType _mytype, SCMProtocol::PeerType _matchtype, unsigned long long _mypeerId, unsigned long long _matchpeerId, DataSocket* lnkSocket, ServerSocket* appListenSock, unsigned int buffer_size, unsigned int size_overhead, std::string app_ip, unsigned int app_port, unsigned char sync, bool _checkhbeat, bool _blockUserSend);
	~SCMRouter();

	bool startRouting();
	bool stopRouting();
	void waitingHeartBeatAck();
	bool isLinkUp();

private:
	unsigned int bufferSize;

	unsigned int sizeOverhead;
	pthread_mutex_t linkSocketMutex;
	DataSocket* linkSocket;
	ServerSocket* userListenSocket;
	DataSocket* apacheSockets[SCM_MAX_CHANNELS];
	SCMHalfChannel* channels[SCM_MAX_CHANNELS];
	pthread_mutex_t channelMutexes[SCM_MAX_CHANNELS];
	SCMProtocol::PeerType myPeerType;
	SCMProtocol::PeerType matchPeerType;
	unsigned long long myPeerId;
	unsigned long long matchPeerId;
	unsigned int appPort;
	std::string appIp;
	unsigned char syncChannel;

	volatile bool linkUp;
	volatile bool apacheSendFail[SCM_MAX_CHANNELS];
	volatile bool incomingRequest[SCM_MAX_CHANNELS];
	volatile bool creatingChannel[SCM_MAX_CHANNELS];

	unsigned long long packetCounter[SCM_MAX_CHANNELS];
	unsigned int inactiveTime[SCM_MAX_CHANNELS];

	pthread_mutex_t hbTimeMutex;
	pthread_mutex_t hbTimeAckMutex;
	pthread_mutex_t waitingHbAckMutex;
	unsigned int hbTime;
	unsigned int hbAckTime;
	bool checkHBeat;
	bool waitingHbAck;
	bool blockUserSend;

	PthreadId factoryThreadId;
	pthread_mutex_t factoryRunningMutex;
	volatile bool stopFactoryThread;
	volatile bool factoryRunning;
	bool startChnFactory();
	void stopChnFactory();
	static void* runChnFactory(void* context);
	void* chnFactoryThread();

	PthreadId auditThreadId;
	pthread_mutex_t auditRunningMutex;
	volatile bool stopAuditThread;
	volatile bool auditRunning;
	bool startChnAudit();
	void stopChnAudit();
	static void* runChnAudit(void* context);
	void* chnAuditThread();

	PthreadId routerThreadId;
	pthread_mutex_t routerRunningMutex;
	volatile bool stopRouterThread;
	volatile bool routerRunning;
	bool startRouter();
	void stopRouter();
	static void* runRouter(void* context);
	void* routerThread();
};

#endif