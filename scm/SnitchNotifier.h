#ifndef _SNITCH_NOTIFIER_H_
#define _SNITCH_NOTIFIER_H_

#include "NotificationReceiver.h"
#include "NotificationSender.h"
#include "SnitchNotification.h"
#include "Socket.h"
#include "Thread.h"
#include <queue>

using namespace std;


class SnitchNotifier
{
public:
	SnitchNotifier(unsigned int receiver_port);
	~SnitchNotifier();

	bool run();
	bool stop();

private:
	NotificationListener* receiver;
	NotificationSender* sender;

	queue<SnitchNotification*>* pendingNotifications;
};

#endif