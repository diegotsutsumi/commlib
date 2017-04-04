#ifndef _THREAD_ID_H_
#define _THREAD_ID_H_

typedef struct PthreadId
{
	bool created;
	pthread_t id;
	pthread_mutex_t id_mutex;
}PthreadId;

#endif