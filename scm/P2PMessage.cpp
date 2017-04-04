#include "P2PMessage.h"
#include <string>
#include <iostream>

using namespace std;
P2PMessage::P2PMessage(unsigned long long _cameraId, unsigned char _type)
{
	cameraId=_cameraId;
	type = _type;
}

P2PMessage::~P2PMessage()
{
}

unsigned char* P2PMessage::serialize(unsigned int* out_size)
{
	unsigned char* ret = new unsigned char[9]();
	unsigned int i=0;

	for(i=0;i<9;i++)
	{
		if(i==0)
		{
			ret[i] = type;
		}
		else if(i>=1 && i<=8)
		{
			ret[i] = cameraIdBytes[i-1];
		}
		else
		{
			*out_size=0;
			return 0;
		}
	}
	*out_size=9;
	return ret;	
}

bool P2PMessage::unserialize(unsigned char* info, unsigned int size)
{
	if(size!=9 || !info)
	{
		return false;
	}

	unsigned int i=0;
	for(i=0;i<9;i++)
	{
		if(i==0)
		{
			type = info[i];
		}
		else if(i>=1 && i<=8)
		{
			cameraIdBytes[i-1] = info[i];
		}
		else
		{
			return false;
		}
	}
	return true;
}

unsigned char P2PMessage::getTypeFromRawInfo(unsigned char* info, unsigned int size)
{
	if(!info || !size)
	{
		return 0;
	}

	return *(info);
}

unsigned char P2PMessage::getMessageType()
{
	return type;
}
unsigned long long P2PMessage::getCameraId()
{
	return cameraId;
}


ConnectRequest::ConnectRequest(unsigned long long __cameraId, unsigned char *_p2pId, unsigned int p2pId_size, unsigned char *_userLogin, unsigned int userLogin_size, unsigned char *_passwordLogin, unsigned int passLogin_size) : P2PMessage(__cameraId, P2P_CONNECT_CAMERA)
{
	unsigned int i=0;
	p2pIdSize = (p2pId_size>=50) ? (50) : (p2pId_size);
	userLoginSize = (userLogin_size>=30) ? (30) : (userLogin_size);
	passwordLoginSize = (passLogin_size>=30) ? (30) : (passLogin_size);

	for(i=0;i<50;i++)
	{
		p2pId[i]=0;
	}
	for(i=0;i<30;i++)
	{
		userLogin[i]=0;
	}
	for(i=0;i<30;i++)
	{
		userLogin[i]=0;
	}

	for(i=0;i<p2pIdSize;i++)
	{
		p2pId[i]=_p2pId[i];
	}
	for(i=0;i<userLoginSize;i++)
	{
		userLogin[i]=_userLogin[i];
	}
	for(i=0;i<passwordLoginSize;i++)
	{
		passwordLogin[i]=_passwordLogin[i];
	}
}
ConnectRequest::ConnectRequest() : P2PMessage(0,P2P_CONNECT_CAMERA)
{
	unsigned int i=0;
	p2pIdSize=0;
	userLoginSize=0;
	passwordLoginSize=0;
	for(i=0;i<50;i++)
	{
		p2pId[i]=0;
	}
	for(i=0;i<30;i++)
	{
		userLogin[i]=0;
	}
	for(i=0;i<30;i++)
	{
		userLogin[i]=0;
	}
}
ConnectRequest::~ConnectRequest()
{
}

void ConnectRequest::abstractMethod()
{
}

unsigned char* ConnectRequest::serialize(unsigned int* out_size)
{
	unsigned char* ret = new unsigned char[127];
	unsigned int i=0;

	for(i=0;i<127;i++)
	{
		if(i==0)
		{
			ret[i] = type;
		}
		else if(i>=1 && i<=8)
		{
			ret[i] = cameraIdBytes[i-1];
		}
		else if(i>=9 && i<=12)
		{
			ret[i] = p2pIdSizeBytes[i-9];
		}
		else if(i>=13 && i<=62)
		{
			ret[i] = p2pId[i-13];
		}
		else if(i>=63 && i<=64)
		{
			ret[i] = userLoginSizeBytes[i-63];
		}
		else if(i>=65 && i<=94)
		{
			ret[i] = userLogin[i-65];
		}
		else if(i>=95 && i<=96)
		{
			ret[i] = passwordLoginSizeBytes[i-95];
		}
		else if(i>=97 && i<=126)
		{
			ret[i] = passwordLogin[i-97];
		}
		else
		{
			*out_size=0;
			return 0;
		}
	}
	*out_size=127;
	return ret;	
}

bool ConnectRequest::unserialize(unsigned char* info, unsigned int size)
{
	if(size!=127 || !info)
	{
		return false;
	}

	unsigned int i=0;
	for(i=0;i<127;i++)
	{
		if(i==0)
		{
			type = info[i];
		}
		else if(i>=1 && i<=8)
		{
			cameraIdBytes[i-1] = info[i];
		}
		else if(i>=9 && i<=12)
		{
			p2pIdSizeBytes[i-9] = info[i];
		}
		else if(i>=13 && i<=62)
		{
			p2pId[i-13] = info[i];
		}
		else if(i>=63 && i<=64)
		{
			userLoginSizeBytes[i-63] = info[i];
		}
		else if(i>=65 && i<=94)
		{
			userLogin[i-65] = info[i];
		}
		else if(i>=95 && i<=96)
		{
			passwordLoginSizeBytes[i-95] = info[i];
		}
		else if(i>=97 && i<=126)
		{
			passwordLogin[i-97] = info[i];
		}
		else
		{
			return false;
		}
	}
	return true;
}

unsigned char* ConnectRequest::getP2PId(unsigned int* out_size)
{
	if(p2pIdSize==0 || p2pIdSize>50)
	{
		*out_size=0;
		return 0;
	}
	unsigned char *ret = new unsigned char[p2pIdSize]();
	for(unsigned int i=0 ; i<p2pIdSize ; i++)
	{
		ret[i] = p2pId[i];
	}
	*out_size = p2pIdSize;
	return ret;
}

unsigned char* ConnectRequest::getUserLogin(unsigned short* out_size)
{
	if(userLoginSize==0 || userLoginSize>30)
	{
		*out_size=0;
		return 0;
	}
	unsigned char *ret = new unsigned char[userLoginSize]();
	for(unsigned int i=0 ; i<userLoginSize ; i++)
	{
		ret[i] = userLogin[i];
	}
	*out_size = userLoginSize;
	return ret;
}

unsigned char* ConnectRequest::getPasswordLogin(unsigned short* out_size)
{
	if(passwordLoginSize==0 || passwordLoginSize>30)
	{
		*out_size=0;
		return 0;
	}
	unsigned char *ret = new unsigned char[passwordLoginSize]();
	for(unsigned int i=0 ; i<passwordLoginSize ; i++)
	{
		ret[i] = passwordLogin[i];
	}
	*out_size = passwordLoginSize;
	return ret;
}

DisconnectRequest::DisconnectRequest(unsigned long long __cameraId) : P2PMessage(__cameraId,P2P_DISCONNECT_CAMERA)
{
}
DisconnectRequest::DisconnectRequest() : P2PMessage(0,P2P_DISCONNECT_CAMERA)
{
}
DisconnectRequest::~DisconnectRequest()
{
}
void DisconnectRequest::abstractMethod()
{
}

GetSnapshotRequest::GetSnapshotRequest(unsigned long long __cameraId) : P2PMessage(__cameraId,P2P_GET_SNAPSHOT)
{
}
GetSnapshotRequest::GetSnapshotRequest() : P2PMessage(0,P2P_GET_SNAPSHOT)
{
}
GetSnapshotRequest::~GetSnapshotRequest()
{
}
void GetSnapshotRequest::abstractMethod()
{
}

CameraOfflineAnswer::CameraOfflineAnswer(unsigned long long __cameraId) : P2PMessage(__cameraId,P2P_CAMERA_OFFLINE)
{
}
CameraOfflineAnswer::CameraOfflineAnswer() : P2PMessage(0,P2P_CAMERA_OFFLINE)
{
}
CameraOfflineAnswer::~CameraOfflineAnswer()
{
}
void CameraOfflineAnswer::abstractMethod()
{
}

CameraConnectingAnswer::CameraConnectingAnswer(unsigned long long __cameraId) : P2PMessage(__cameraId,P2P_CAMERA_CONNECTING)
{
}
CameraConnectingAnswer::CameraConnectingAnswer() : P2PMessage(0,P2P_CAMERA_CONNECTING)
{
}
CameraConnectingAnswer::~CameraConnectingAnswer()
{
}
void CameraConnectingAnswer::abstractMethod()
{
}

SnapshotAnswer::SnapshotAnswer(unsigned long long __cameraId, unsigned long long _p2pSnapSize, unsigned char * _snap) : P2PMessage(__cameraId,P2P_SNAPSHOT_CONTENT)
{
	p2pSnapSize = _p2pSnapSize;
	snapshot = new unsigned char[p2pSnapSize];
	for(unsigned int i=0;i<p2pSnapSize;i++)
	{
		snapshot[i] = _snap[i];
	}
}
SnapshotAnswer::SnapshotAnswer() : P2PMessage(0,P2P_SNAPSHOT_CONTENT)
{
	p2pSnapSize = 0;
	snapshot = 0;
}

SnapshotAnswer::~SnapshotAnswer()
{
	if(snapshot)
	{
		delete snapshot;
		snapshot =0;
	}
}

void SnapshotAnswer::abstractMethod()
{
}

unsigned char* SnapshotAnswer::serialize(unsigned int* out_size)
{
	unsigned char* ret = new unsigned char[9+8+p2pSnapSize]();
	unsigned int i=0;

	for(i=0;i<17;i++)
	{
		if(i==0)
		{
			ret[i] = type;
		}
		else if(i>=1 && i<=8)
		{
			ret[i] = cameraIdBytes[i-1];
		}
		else if(i>=9 && i<=16)
		{
			ret[i] = p2pSnapSizeBytes[i-9];
		}
		else
		{
			*out_size=0;
			return 0;
		}
	}
	for(i=0;i<p2pSnapSize;i++)
	{
		ret[i+17] = snapshot[i];
	}

	*out_size=9+8+p2pSnapSize;
	return ret;	
}

bool SnapshotAnswer::unserialize(unsigned char* info, unsigned int size)
{
	if(size<17 || !info)
	{
		return false;
	}

	unsigned int i=0;
	for(i=0;i<17;i++)
	{
		if(i==0)
		{
			type = info[i];
		}
		else if(i>=1 && i<=8)
		{
			cameraIdBytes[i-1] = info[i];
		}
		else if(i>=9 && i<=16)
		{
			p2pSnapSizeBytes[i-9] = info[i];
		}
		else
		{
			return false;
		}
	}

	if(snapshot)
	{
		delete snapshot;
		snapshot=0;
	}

	snapshot = new unsigned char[p2pSnapSize];
	for(i=0;i<p2pSnapSize;i++)
	{
		snapshot[i] = info[i+17];
	}
	return true;
}

unsigned char* SnapshotAnswer::getSnapshot(unsigned long long * out_size)
{
	*out_size = p2pSnapSize;
	return snapshot;
}

AlreadyRequestedAnswer::AlreadyRequestedAnswer(unsigned long long __cameraId) : P2PMessage(__cameraId,P2P_ALREADY_REQUESTED)
{
}
AlreadyRequestedAnswer::AlreadyRequestedAnswer() : P2PMessage(0,P2P_ALREADY_REQUESTED)
{
}
AlreadyRequestedAnswer::~AlreadyRequestedAnswer()
{
}
void AlreadyRequestedAnswer::abstractMethod()
{
}

ErrorAnswer::ErrorAnswer(unsigned long long __cameraId) : P2PMessage(__cameraId,P2P_ERROR)
{
}
ErrorAnswer::ErrorAnswer() : P2PMessage(0,P2P_ERROR)
{
}
ErrorAnswer::~ErrorAnswer()
{
}
void ErrorAnswer::abstractMethod()
{
}