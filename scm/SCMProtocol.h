#ifndef _SCMPROTOCOL_H_
#define _SCMPROTOCOL_H_

#define SCM_MAX_CHANNELS 200
#define DEFAULT_HTTPS_SERVER_PORT 2056
#define DEFAULT_SSH_SERVER_PORT 2057
#define DEFAULT_RTSP_SERVER_PORT 2058

namespace SCMProtocol
{
	const unsigned char SCMStartKey[4] = {0x1A,0x8B,0xA7,0x12};
	const unsigned char SCMEndKey[4] = {0x1B,0x8C,0xA8,0x22};
	const unsigned int SCMStartHeaderSize = 23;
	const unsigned int SCMEndHeaderSize = 4;
	const unsigned int SCMHeaderSize = 27;
	typedef enum 
	{
		InvalidType=0,
		Server,
		Client
	}PeerType;

	typedef enum 
	{
		InvalidCommand=0,
		Register,
		HeartBeat,
		Route,
		SyncListen,
		SyncDelete,
		Ack,
		Nack,
		HeartBeatAck
	}Command;
}

#endif