#ifndef _SNITCH_NOTIFICATION_H_
#define _SNITCH_NOTIFICATION_H_

#include <string>
using namespace std;

#define MOTION_NOTIFICATION_TYPE 1
#define SIGNALLOSS_NOTIFICATION_TYPE 2

class SnitchNotification
{
public:
	SnitchNotification();
	virtual ~SnitchNotification();

	virtual std::string getShellCommand()=0;
	virtual unsigned char* serialize(unsigned int* out_size)=0;
	virtual bool unserialize(unsigned char* info, unsigned int size)=0;

	static unsigned char getTypeFromRawInfo(unsigned char* info, unsigned int size);

	unsigned char getNotificationType();

protected:
	unsigned char type;
};

class MotionNotification : public SnitchNotification
{
public:
	MotionNotification(unsigned long long _eventId, unsigned long long _frameId);
	MotionNotification();
	~MotionNotification();

	unsigned long long getEventId();
	unsigned long long getFrameId();

	std::string getShellCommand();
	unsigned char* serialize(unsigned int* out_size);
	bool unserialize(unsigned char* info, unsigned int size);

private:
	union
	{
		unsigned long long eventId;
		unsigned char eventIdBytes[8];
	};
	union
	{
		unsigned long long frameId;
		unsigned char frameIdBytes[8];
	};
};

class SignalLossNotification : public SnitchNotification
{
public:
	SignalLossNotification(unsigned long long _cameraId);
	SignalLossNotification();
	~SignalLossNotification();

	unsigned long long getCameraId();

	std::string getShellCommand();
	unsigned char* serialize(unsigned int* out_size);
	bool unserialize(unsigned char* info, unsigned int size);
	
private:
	union
	{
		unsigned long long cameraId;
		unsigned char cameraIdBytes[8];
	};
};

#endif