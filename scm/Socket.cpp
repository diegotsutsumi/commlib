// Implementation of the Socket class.

#include "Socket.h"
#include "Exception.h"
#include <errno.h>
#include <fcntl.h>
#include <exception>
#include <sys/time.h>
#include <iostream>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>

Socket::Socket()
{
	m_sock = -1;
	connected = false;
	set_non_blocking(false);
	memset(&m_addr, 0, sizeof(m_addr));
}
Socket::~Socket()
{
	if(is_valid())
	{
		::close(m_sock);
	}
}
bool Socket::is_valid()
{
	return m_sock != -1;
}

void Socket::set_non_blocking(bool b)
{
	int opts=-1;

	opts = fcntl(m_sock, F_GETFL);
	if(opts < 0)
	{
		return;
	}

	if(b)
	{
		opts = (opts | O_NONBLOCK);
	}
	else
	{
		opts = (opts & ~O_NONBLOCK);
	}
	fcntl(m_sock, F_SETFL, opts);
}
bool Socket::create(unsigned int port)
{
	if(is_valid())
	{
		::close(m_sock);
	}

	m_sock = ::socket(AF_INET, SOCK_STREAM, 0);

	if (!is_valid())
	{
		return false;
	}

	// TIME_WAIT - argh
	int on = 1;
	if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*) &on, sizeof(on)) == -1)
	{
		return false;
	}
	if (setsockopt(m_sock, SOL_SOCKET, SO_KEEPALIVE, (const char*) &on, sizeof(on)) == -1)
	{
		return false;
	}

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

	if(setsockopt (m_sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) == -1)
	{
		return false;
	}

	return true;
}

int Socket::getLocalPort()
{
	struct sockaddr_in soin;
	int addrlen = sizeof(soin);
	if(getsockname(m_sock, (struct sockaddr *)&soin, (socklen_t*)&addrlen) == 0 && soin.sin_family == AF_INET && addrlen == sizeof(soin))
	{
		return ntohs(soin.sin_port);
	}
	else
	{
		return -1;
	}
}

char * Socket::getLocalIp()
{
	struct ifreq ifr;

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
	ioctl(m_sock, SIOCGIFADDR, &ifr);

	return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
}





UDPSocket::UDPSocket(unsigned int port)
{
	pthread_mutex_init(&sendMutex,NULL);
	if(!create(port))
	{
		throw SocketException("Couldn't create Socket.");
	}
}

UDPSocket::~UDPSocket()
{
	pthread_mutex_destroy(&sendMutex);
}

SocketType UDPSocket::getType()
{
	return UDP;
}

bool UDPSocket::create(int port)
{
	m_sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(!lnx_bind((unsigned int)port))
	{
		return false;
	}

	if(!is_valid())
	{
		return false;
	}
	int on = 1;
	if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEPORT, (const char*) &on, sizeof(on)) == -1)
	{
		return false;
	}
	if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*) &on, sizeof(on)) == -1)
	{
		return false;
	}
	return true;
}

/*bool UDPSocket::bindRemoteAddr(const std::string host, unsigned int port)
{
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(port);
	if (inet_aton(host.c_str(), &m_addr.sin_addr) == 0) 
	{
		return false;
	}
	return true;
}*/

long int UDPSocket::receiveData(unsigned char * data, size_t len, unsigned int timeout, char ** out_senderHost, int * out_senderPort, int * out_errno)
{
	int status=0;
	struct sockaddr_in sender;
	socklen_t sendsize = sizeof(sender);
	bzero(&sender, sizeof(sender));

	if(!is_valid())
	{
		return -1;
	}

	memset(data, 0, len);

	if(timeout>4 && timeout<1001)
	{
		set_non_blocking(true);
		/*Preparing Select Timeout*/
		fd_set rfds;
		struct timeval tv;
		int retval;
		FD_ZERO(&rfds);
		FD_SET(m_sock, &rfds);
		tv.tv_sec = 0;
		tv.tv_usec = timeout*1000;
		retval = ::select(m_sock+1, &rfds, NULL, NULL, &tv);
		if(retval==-1)
		{
			status = ::recvfrom(m_sock, data, len, 0, (struct sockaddr *) &sender, &sendsize); //status = ::recv(m_sock, data, len, 0);
			*out_senderHost = inet_ntoa(sender.sin_addr);
			*out_senderPort = ntohs(sender.sin_port);
			*out_errno=errno;
			return status;
		}
		else if(retval)
		{
			status = ::recvfrom(m_sock, data, len, 0, (struct sockaddr *) &sender, &sendsize); //status = ::recv(m_sock, data, len, 0);
			*out_senderHost = inet_ntoa(sender.sin_addr);
			*out_senderPort = ntohs(sender.sin_port);
			*out_errno=errno;
			return (long int) status;
		}
		else
		{
			*out_errno=EWOULDBLOCK;
			return -1;
		}
	}
	else
	{
		set_non_blocking(false);
		status = ::recvfrom(m_sock, data, len, 0, (struct sockaddr *) &sender, &sendsize); //status = ::recv(m_sock, data, len, 0);
		*out_senderHost = inet_ntoa(sender.sin_addr);
		*out_senderPort = ntohs(sender.sin_port);
		*out_errno=errno;
		return (long int) status;
	}
}

bool UDPSocket::sendData(unsigned char * data, size_t len, const std::string host, unsigned int port)
{
	int status = -1;
	int _errno = 0;
	int tries=0;

	sockaddr_in to_addr;

	to_addr.sin_family = AF_INET;
	to_addr.sin_port = htons(port);
	if (inet_aton(host.c_str(), &to_addr.sin_addr) == 0) 
	{
		return false;
	}

	if(!data || !len)
	{
		return true;
	}

	if(is_valid())
	{
		set_non_blocking(false);
		pthread_mutex_lock(&sendMutex);
		while(len>0)
		{
			status = ::sendto(m_sock, data, len, 0, (struct sockaddr *)(&to_addr), sizeof(to_addr));
			if(status==-1)
			{
				pthread_mutex_unlock(&sendMutex);
				if(tries>=5)
				{
					return false;
				}
				_errno=errno;
				if(_errno==EWOULDBLOCK || _errno==EAGAIN)
				{
					tries++;
					continue;
				}
				else
				{
					return false;
				}
			}
			data = data + status;
			len = len - status;
			tries=0;
		}
		pthread_mutex_unlock(&sendMutex);
	}
	else
	{
		return false;
	}

	return true;
}

bool UDPSocket::lnx_bind(unsigned int port)
{
	
	if(!is_valid())
	{
		return false;
	}

	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = INADDR_ANY;
	m_addr.sin_port = htons ( port );

	int bind_return = ::bind(m_sock, (struct sockaddr *) &m_addr, sizeof(m_addr));

	if (bind_return == -1)
	{
		return false;
	}

	return true;
}


ServerSocket::ServerSocket(unsigned int port)
{
	if(!create(port))
	{
		throw SocketException("Couldn't create Socket.");
	}
	if(!lnx_bind(port))
	{
		throw SocketException("Couldn't bind port: to the address.");
	}
	if(!lnx_listen())
	{
		throw SocketException("Couldn't set as listening socket.");
	}
}
ServerSocket::~ServerSocket()
{
}
bool ServerSocket::lnx_bind(unsigned int port)
{
	
	if(!is_valid())
	{
		return false;
	}

	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = INADDR_ANY;
	m_addr.sin_port = htons ( port );

	int bind_return = ::bind(m_sock, (struct sockaddr *) &m_addr, sizeof(m_addr));

	if (bind_return == -1)
	{
		return false;
	}

	return true;
}
bool ServerSocket::lnx_listen()
{
	if (!is_valid())
	{
		return false;
	}

	int listen_return = ::listen(m_sock, MAXPENDING);

	if (listen_return == -1)
	{
		return false;
	}
	return true;
}

bool ServerSocket::lnx_accept(int * new_socket_desc)
{
	int addr_length = sizeof(m_addr);
	*new_socket_desc = ::accept(m_sock, (sockaddr*) &m_addr, (socklen_t*) &addr_length);

	if (*new_socket_desc <= 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

DataSocket * ServerSocket::startListening(unsigned int timeout, bool* out_timeover)
{
	int data_sock_desc;

	*out_timeover = false;
	if(timeout>4 && timeout<1001)
	{
		set_non_blocking(true);
		/*Preparing Select Timeout*/
		fd_set rfds;
		struct timeval tv;
		int retval;
		FD_ZERO(&rfds);
		FD_SET(m_sock, &rfds);
		tv.tv_sec = 0;
		tv.tv_usec = timeout*1000;
		retval = ::select(m_sock+1, &rfds, NULL, NULL, &tv);

		if(retval==-1)
		{
			return 0;
		}
		else if(retval)
		{
			if(!lnx_accept(&data_sock_desc))
			{
				return 0;
			}
			if(data_sock_desc<=0)
			{
				return 0;
			}
			return new DataSocket(data_sock_desc);
		}
		else
		{
			*out_timeover = true;
			return 0;
		}
	}
	else
	{
		set_non_blocking(false);
		if(!lnx_accept(&data_sock_desc))
		{
			return 0;
		}
		if(data_sock_desc<=0)
		{
			return 0;
		}

		return new DataSocket(data_sock_desc);
	}
}

SocketType ServerSocket::getType()
{
	return Server;
}


DataSocket::DataSocket()
{
	sentNum=0;
	receivedNum=0;
	pthread_mutex_init(&sendMutex,NULL);
	pthread_mutex_init(&nonBlockMutex,NULL);
}

DataSocket::DataSocket(int sock_desc)
{
	sentNum=0;
	receivedNum=0;
	m_sock = sock_desc;
	if(is_valid())
	{
		connected = true;
	}
	else
	{
		connected = false;
	}
	memset(&m_addr, 0, sizeof(m_addr));
	pthread_mutex_init(&sendMutex,NULL);
	pthread_mutex_init(&nonBlockMutex,NULL);
}

DataSocket::~DataSocket()
{
	pthread_mutex_destroy(&sendMutex);
	pthread_mutex_destroy(&nonBlockMutex);
}

bool DataSocket::sendData(unsigned char * data, size_t len, unsigned int timeout, bool *out_timeover)
{
	int status = -1;
	int _errno = 0;
	int tries=0;

	if(!data || !len)
	{
		*out_timeover=false;
		return true;
	}

	if(!connected)
	{
		*out_timeover=false;
		return false;
	}

	if(is_valid())
	{
		pthread_mutex_lock(&nonBlockMutex);
		if(timeout>0 && timeout<60)
		{
			set_non_blocking(true);
			/*Preparing Select Timeout*/
			fd_set rfds;
			struct timeval tv;
			int retval;
			FD_ZERO(&rfds);
			FD_SET(m_sock, &rfds);
			tv.tv_sec = timeout;
			tv.tv_usec = 0;
			pthread_mutex_unlock(&nonBlockMutex);
			pthread_mutex_lock(&sendMutex);
			while(len>0)
			{
				retval = ::select(m_sock+1, NULL, &rfds, NULL, &tv);
				if(retval==-1)
				{
					std::cout << "Error in select " << errno << std::endl;
					pthread_mutex_unlock(&sendMutex);
					*out_timeover=false;
					return false;
				}
				else if(retval)
				{
					status = ::send(m_sock, data, len, MSG_NOSIGNAL);
					if(status==-1)
					{
						std::cout << "Error in send " << errno << std::endl;
						pthread_mutex_unlock(&sendMutex);
						*out_timeover=false;
						return false;
					}
					data = data + status;
					len = len - status;
				}
				else
				{
					std::cout << "Select returned zero" << std::endl;
					*out_timeover=true;
					pthread_mutex_unlock(&sendMutex);
					return false;
				}
			}
			pthread_mutex_unlock(&sendMutex);			
		}
		else
		{
			set_non_blocking(false);
			pthread_mutex_unlock(&nonBlockMutex);

			pthread_mutex_lock(&sendMutex);
			while(len>0)
			{
				status = ::send(m_sock, data, len, MSG_NOSIGNAL);
				if(status==-1)
				{
					pthread_mutex_unlock(&sendMutex);
					if(tries>=5)
					{
						*out_timeover=false;
						return false;
					}
					_errno=errno;
					if(_errno==EWOULDBLOCK || _errno==EAGAIN)
					{
						std::cout << "Blocking send errno WouldBlock " << std::endl;
						tries++;
						continue;
					}
					else
					{
						std::cout << "Blocking send errno " << errno << std::endl;
						*out_timeover=false;
						return false;
					}
				}
				data = data + status;
				len = len - status;
				tries=0;
			}
			pthread_mutex_unlock(&sendMutex);
		}
	}
	else
	{
		*out_timeover=false;
		return false;
	}

	*out_timeover=false;
	return true;
}

long int DataSocket::receiveData(unsigned char * data, size_t len, unsigned int timeout, int *out_errno)
{
	int status=0;
	if(!connected || !is_valid())
	{
		return -1;
	}

	memset(data, 0, len);

	pthread_mutex_lock(&nonBlockMutex);
	if(timeout>4 && timeout<1001)
	{
		set_non_blocking(true);
		/*Preparing Select Timeout*/
		fd_set rfds;
		struct timeval tv;
		int retval;
		FD_ZERO(&rfds);
		FD_SET(m_sock, &rfds);
		tv.tv_sec = 0;
		tv.tv_usec = timeout*1000;
		pthread_mutex_unlock(&nonBlockMutex);
		retval = ::select(m_sock+1, &rfds, NULL, NULL, &tv);
		if(retval==-1)
		{
			status = ::recv(m_sock, data, len, 0);
			*out_errno=errno;
			return status;
		}
		else if(retval)
		{
			status = ::recv(m_sock, data, len, 0);
			*out_errno=errno;
			return (long int) status;
		}
		else
		{
			*out_errno=EWOULDBLOCK;
			return -1;
		}
	}
	else
	{
		set_non_blocking(false);
		pthread_mutex_unlock(&nonBlockMutex);
		status = ::recv(m_sock, data, len, 0);
		*out_errno=errno;
		return (long int) status;
	}
}

SocketType DataSocket::getType()
{
	return Data;
}

ClientSocket::ClientSocket()
{
	if(!create(0))
	{
		throw SocketException("Couldn't create Socket.");
	}
}
ClientSocket::~ClientSocket()
{
}
bool ClientSocket::connectTo(const std::string host, const int port)
{
	if(!is_valid())
	{
		return false;
	}

	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(port);

	int status = inet_pton (AF_INET, host.c_str(), &m_addr.sin_addr);

	if(errno == EAFNOSUPPORT)
	{
		return false;
	}

	status = ::connect(m_sock, (sockaddr*) &m_addr, sizeof(m_addr));

	if (status == 0)
	{
		connected = true;
		return true;
	}
	else
	{
		return false;
	}
}

SocketType ClientSocket::getType()
{
	return Client;
}



HalfDuplexChannel::HalfDuplexChannel(DataSocket * _in, DataSocket * _out, unsigned int buffer_size, unsigned int size_overhead)
{
	if(!_in || !_out)
	{
		throw ChannelException("Invalid Sockets");
	}
	else if(!_in->is_valid() || !_out->is_valid())
	{
		throw ChannelException("Invalid Sockets");
	}

	halted = false;
	runningThread = false;
	in = _in;
	out = _out;
	bufferSize = buffer_size;
	sizeOverhead = size_overhead;
	packetCounter=0;

	threadId.id=0;
	threadId.created=false;
	pthread_mutex_init(&threadId.id_mutex,NULL);	

	pthread_mutex_init(&runningMutex,NULL);
	pthread_mutex_init(&haltedMutex,NULL);
}
HalfDuplexChannel::~HalfDuplexChannel()
{
	//stopChannel();

	pthread_mutex_destroy(&runningMutex);
	pthread_mutex_destroy(&haltedMutex);
	pthread_mutex_destroy(&threadId.id_mutex);
}

void* HalfDuplexChannel::runThread(void* context)
{
	return ((HalfDuplexChannel*)context)->threadFunc();
}

bool HalfDuplexChannel::startChannel()
{
	stopThread=false;

	pthread_mutex_lock(&threadId.id_mutex);
	bool ret = (pthread_create(&threadId.id, NULL, runThread, this) == 0);

	if(ret)
	{
		threadId.created=true;
	}

	pthread_mutex_unlock(&threadId.id_mutex);
	return ret;
}

void HalfDuplexChannel::stopChannel()
{
	stopThread=true;

	pthread_mutex_lock(&threadId.id_mutex);
	if(threadId.created)
	{
		pthread_join(threadId.id,NULL);
		threadId.created=false;
	}
	pthread_mutex_unlock(&threadId.id_mutex);

}

long int HalfDuplexChannel::processBuffer(unsigned char* buffer, long int _recvSize)
{
	return _recvSize;
}

void *HalfDuplexChannel::threadFunc() //TODO: Consider trying to restabilish communication itself
{
	pthread_mutex_lock(&runningMutex);
	runningThread = true;
	pthread_mutex_unlock(&runningMutex);

	pthread_mutex_lock(&haltedMutex);
	halted = false;
	pthread_mutex_unlock(&haltedMutex);


	unsigned long long buffMemory = (bufferSize+sizeOverhead) + 100;

	unsigned char buffer[buffMemory];
	memset(buffer,0,sizeof(buffer));

	//unsigned char * buffer = new unsigned char[buffMemory];

	long int recvSize=0;
	//bool connectionActive=false;
	bool sendOk=false;
	int _errno=0;
	unsigned char recvErrorNum=0;
	bool timeover;

	while(1)
	{
		if(stopThread)
		{
			break;
		}
		if(recvErrorNum>3)
		{
			break;
		}
		recvSize = (long int)(in->receiveData(buffer, bufferSize,500,&_errno));
		
		if(recvSize==-1 && _errno!=EWOULDBLOCK && _errno!=EAGAIN)
		{
			recvErrorNum++;
			//connectionActive = false;
			//socket error
			//break;
		}
		else if(recvSize==0)
		{
			recvErrorNum++;
			//Connection Loss
			//connectionActive = false;
			//break;
		}
		else if(recvSize>0)
		{
			recvErrorNum=0;
			recvSize = processBuffer(buffer, recvSize); //TODO: check returned Buffer size
			sendOk = false;
			for(int i=0; i<2; i++)
			{
				sendOk = out->sendData(buffer, recvSize, 0, &timeover);
				if(sendOk)
				{
					break;
				}
			}
			if(!sendOk)
			{
				break;
			}
			packetCounter++;
		}
		else
		{
			recvErrorNum=0;
			//No data received
		}
	}
	
	pthread_mutex_lock(&haltedMutex);
	halted = true;
	pthread_mutex_unlock(&haltedMutex);
	/*if(buffer)
	{
		delete[] buffer;
		buffer=0;
	}*/
	pthread_mutex_lock(&runningMutex);
	runningThread = false;
	pthread_mutex_unlock(&runningMutex);
	return 0;
}

bool HalfDuplexChannel::isHalted()
{
	bool retHalted;

	pthread_mutex_lock(&haltedMutex);
	retHalted = halted;
	pthread_mutex_unlock(&haltedMutex);
	
	return retHalted;
}

unsigned long long HalfDuplexChannel::getPacketCounter()
{
	return packetCounter;
}

void HalfDuplexChannel::clearPacketCounter()
{
	packetCounter = 0;
}


FullDuplexChannel::FullDuplexChannel(DataSocket* _first, DataSocket* _second, unsigned int buffer_size, unsigned int size_overhead)
{
	first = new HalfDuplexChannel(_first,_second,buffer_size,size_overhead);
	second = new HalfDuplexChannel(_second,_first,buffer_size,size_overhead);
}

FullDuplexChannel::~FullDuplexChannel()
{
	delete first;
	delete second;
}

void FullDuplexChannel::getPacketCounters(unsigned long long *out_firstCounter, unsigned long long *out_secondCounter)
{
	*out_firstCounter=first->getPacketCounter();
	*out_secondCounter=second->getPacketCounter();
}

void FullDuplexChannel::clearPacketCounters()
{
	first->clearPacketCounter();
	second->clearPacketCounter();
}

bool FullDuplexChannel::startChannel()
{
	bool ret = true;
	ret &= first->startChannel();
	ret &= second->startChannel();
	return ret;
}

bool FullDuplexChannel::stopChannel()
{
	first->stopChannel();
	second->stopChannel();
	return true;
}

bool FullDuplexChannel::isHalted()
{
	bool ret = true;
	ret &= first->isHalted();
	ret &= second->isHalted();
	return ret;
}