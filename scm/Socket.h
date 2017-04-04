// Definition of the Socket class

#ifndef _SOCKET_H_
#define _SOCKET_H_


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <pthread.h>

#include "Thread.h"

#define MAXHOSTNAME	200
#define MAXPENDING	5

typedef enum
{
	Server=1,
	Data,
	Client,
	UDP,
	None
}SocketType;

class Socket
{
public:
	Socket();
	virtual ~Socket();
	bool is_valid();
	virtual SocketType getType()=0; //Just to make it an abstract Class
	int getLocalPort();
	char * getLocalIp();

protected:
	int m_sock;
	sockaddr_in m_addr;
	bool connected;

	void set_non_blocking (bool b);
	virtual bool create(unsigned int port);
};

class UDPSocket : public Socket
{
public:
	UDPSocket(unsigned int port);
	~UDPSocket();
	SocketType getType();

	//bool bindRemoteAddr(const std::string host, unsigned int port);
	bool sendData(unsigned char * data, size_t len, const std::string host, unsigned int port);
	long int receiveData(unsigned char * data, size_t len, unsigned int timeout, char ** out_senderHost, int * out_senderPort, int * out_errno);

private:
	pthread_mutex_t sendMutex;
	bool create(int port);
	bool lnx_bind(unsigned int port);
};

class DataSocket : public Socket
{
public:
	DataSocket(int sock_desc); //Valid connected sockets
	virtual ~DataSocket();
	SocketType getType();

	bool sendData(unsigned char * data, size_t len, unsigned int timeout, bool *out_timeover);
	long int receiveData(unsigned char * data, size_t len, unsigned int timeout, int * out_errno);

private:
	pthread_mutex_t nonBlockMutex;
	pthread_mutex_t sendMutex;
	unsigned int sentNum;
	unsigned int receivedNum;

protected:
	DataSocket();
};

class ServerSocket : public Socket
{
public:
	ServerSocket(unsigned int port);
	~ServerSocket();
	SocketType getType();

	DataSocket* startListening(unsigned int timeout, bool * out_timeover);

private:
	bool lnx_bind(unsigned int port);
	bool lnx_listen();
	bool lnx_accept(int * new_socket_desc);
};

class ClientSocket : public DataSocket
{
public:
	ClientSocket();
	~ClientSocket();
	SocketType getType();

	bool connectTo(const std::string host, const int port);
};

class HalfDuplexChannel
{
public:
	HalfDuplexChannel(DataSocket * _in, DataSocket * _out, unsigned int buffer_size, unsigned int size_overhead);
	virtual ~HalfDuplexChannel();
	bool startChannel();
	void stopChannel();
	bool isHalted();
	unsigned long long getPacketCounter();
	void clearPacketCounter();

protected:
	DataSocket * in;
	DataSocket * out;

	unsigned int bufferSize;
	unsigned int sizeOverhead;

	unsigned long long packetCounter;
	
	PthreadId threadId;
	pthread_mutex_t runningMutex;
	pthread_mutex_t haltedMutex;
	volatile bool halted;
	volatile bool stopThread;
	volatile bool runningThread;
	void* threadFunc();
	static void* runThread(void* context);
	virtual long int processBuffer(unsigned char* buffer, long int _recvSize);
};

class FullDuplexChannel
{
public:
	FullDuplexChannel(DataSocket* _first, DataSocket* _second, unsigned int buffer_size, unsigned int size_overhead);
	~FullDuplexChannel();

	bool startChannel();
	bool stopChannel();
	bool isHalted();
	void getPacketCounters(unsigned long long *out_firstCounter, unsigned long long *out_secondCounter);
	void clearPacketCounters();

private:
	HalfDuplexChannel* first;
	HalfDuplexChannel* second;
};


#endif
