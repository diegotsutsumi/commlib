#include "SnitchNotification.h"

SnitchNotification::SnitchNotification()
{
}

SnitchNotification::~SnitchNotification()
{
}

unsigned char SnitchNotification::getTypeFromRawInfo(unsigned char* info, unsigned int size)
{
	if(!info || !size)
	{
		return 0;
	}

	return *(info);
}

unsigned char SnitchNotification::getNotificationType()
{
	return type;
}

MotionNotification::MotionNotification(unsigned long long _eventId, unsigned long long _frameId)
{
	type = MOTION_NOTIFICATION_TYPE;
	eventId=_eventId;
	frameId=_frameId;
}
MotionNotification::MotionNotification()
{
	type = MOTION_NOTIFICATION_TYPE;
	eventId=0;
	frameId=0;
}
MotionNotification::~MotionNotification()
{
}

unsigned long long MotionNotification::getEventId()
{
	return eventId;
}

unsigned long long MotionNotification::getFrameId()
{
	return frameId;
}

std::string MotionNotification::getShellCommand()
{
	std::string ret = "sudo /usr/bin/php /var/www/techideas/index.php snitchprivate notifyAlarm " + std::to_string(eventId) + " " + std::to_string(frameId);
	return ret;
}

unsigned char* MotionNotification::serialize(unsigned int* out_size)
{
	unsigned char* ret = new unsigned char[17];
	unsigned int i=0;

	for(i=0;i<17;i++)
	{
		if(i==0)
		{
			ret[i] = type;
		}
		else if(i>=1 && i<=8)
		{
			ret[i] = eventIdBytes[i-1];
		}
		else if(i>=9 && i<=16)
		{
			ret[i] = frameIdBytes[i-9];
		}
		else
		{
			*out_size=0;
			return 0;
		}
	}
	*out_size=17;
	return ret;	
}

bool MotionNotification::unserialize(unsigned char* info, unsigned int size)
{
	if(size!=17 || !info)
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
			eventIdBytes[i-1] = info[i];
		}
		else if(i>=9 && i<=16)
		{
			frameIdBytes[i-9] = info[i];
		}
		else
		{
			return false;
		}
	}
	return true;
}

SignalLossNotification::SignalLossNotification(unsigned long long _cameraId)
{
	type = SIGNALLOSS_NOTIFICATION_TYPE;
	cameraId=_cameraId;
}

SignalLossNotification::SignalLossNotification()
{
	type = SIGNALLOSS_NOTIFICATION_TYPE;
	cameraId=0;
}
SignalLossNotification::~SignalLossNotification()
{
}

unsigned long long SignalLossNotification::getCameraId()
{
	return cameraId;
}

std::string SignalLossNotification::getShellCommand()
{
	std::string ret = ";"; //Not implemented yet
	return ret;
}

unsigned char* SignalLossNotification::serialize(unsigned int* out_size)
{
	unsigned char* ret = new unsigned char[9];
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

bool SignalLossNotification::unserialize(unsigned char* info, unsigned int size)
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