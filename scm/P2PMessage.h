#ifndef _P2P_REQUEST_H_
#define _P2P_REQUEST_H_

#include <string>
using namespace std;

//Requests
#define P2P_CONNECT_CAMERA 1
#define P2P_DISCONNECT_CAMERA 2
#define P2P_GET_SNAPSHOT 3

//Answers
#define P2P_CAMERA_OFFLINE 5
#define P2P_CAMERA_CONNECTING 6
#define P2P_SNAPSHOT_CONTENT 7
#define P2P_ALREADY_REQUESTED 8
#define P2P_ERROR 9

class P2PMessage
{
public:
	P2PMessage(unsigned long long _cameraId, unsigned char _type);
	virtual ~P2PMessage();

	virtual unsigned char* serialize(unsigned int* out_size);
	virtual bool unserialize(unsigned char* info, unsigned int size);
	virtual void abstractMethod()=0;

	static unsigned char getTypeFromRawInfo(unsigned char* info, unsigned int size);

	unsigned char getMessageType();
	unsigned long long getCameraId();

protected:
	unsigned char type;
	union
	{
		unsigned long long cameraId;
		unsigned char cameraIdBytes[8];
	};
};

class ConnectRequest : public P2PMessage
{
public:
	ConnectRequest(unsigned long long __cameraId, unsigned char *_p2pId, unsigned int p2pId_size, unsigned char *_userLogin, unsigned int userLogin_size, unsigned char *_passwordLogin, unsigned int passLogin_size);
	ConnectRequest();
	~ConnectRequest();

	void abstractMethod();
	unsigned char* serialize(unsigned int* out_size);
	bool unserialize(unsigned char* info, unsigned int size);

	unsigned char* getP2PId(unsigned int* out_size);
	unsigned char* getUserLogin(unsigned short* out_size);
	unsigned char* getPasswordLogin(unsigned short* out_size);

private:
	union
	{
		unsigned int p2pIdSize;
		unsigned char p2pIdSizeBytes[4];
	};
	unsigned char p2pId[50];
	union
	{
		unsigned short userLoginSize;
		unsigned char userLoginSizeBytes[2];
	};
	unsigned char userLogin[30];
	union
	{
		unsigned short passwordLoginSize;
		unsigned char passwordLoginSizeBytes[2];
	};
	unsigned char passwordLogin[30];
};

class DisconnectRequest : public P2PMessage
{
public:
	DisconnectRequest(unsigned long long __cameraId);
	DisconnectRequest();
	~DisconnectRequest();

	void abstractMethod();
};

class GetSnapshotRequest : public P2PMessage
{
public:
	GetSnapshotRequest(unsigned long long __cameraId);
	GetSnapshotRequest();
	~GetSnapshotRequest();

	void abstractMethod();
};

class CameraOfflineAnswer : public P2PMessage
{
public:
	CameraOfflineAnswer(unsigned long long __cameraId);
	CameraOfflineAnswer();
	~CameraOfflineAnswer();

	void abstractMethod();
};

class CameraConnectingAnswer : public P2PMessage
{
public:
	CameraConnectingAnswer(unsigned long long __cameraId);
	CameraConnectingAnswer();
	~CameraConnectingAnswer();

	void abstractMethod();
};

class SnapshotAnswer : public P2PMessage
{//Varible Size Packet
public:
	SnapshotAnswer(unsigned long long __cameraId, unsigned long long _p2pSnapSize, unsigned char * _snap);
	SnapshotAnswer();
	~SnapshotAnswer();

	void abstractMethod();	
	unsigned char* serialize(unsigned int* out_size);
	bool unserialize(unsigned char* info, unsigned int size);

	unsigned char* getSnapshot(unsigned long long * out_size);

private:
	union
	{
		unsigned long long p2pSnapSize;
		unsigned char p2pSnapSizeBytes[8];
	};

	unsigned char * snapshot;
};

class AlreadyRequestedAnswer : public P2PMessage
{
public:
	AlreadyRequestedAnswer(unsigned long long __cameraId);
	AlreadyRequestedAnswer();
	~AlreadyRequestedAnswer();

	void abstractMethod();
};

class ErrorAnswer : public P2PMessage
{
public:
	ErrorAnswer(unsigned long long __cameraId);
	ErrorAnswer();
	~ErrorAnswer();

	void abstractMethod();
};
#endif