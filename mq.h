#ifndef MQ_H
#define MQ_H

#include <pthread.h>

struct Queue 
{
	pthread_mutex_t lock;
	pthread_cond_t wait_room;
	pthread_cond_t wait_data;
	unsigned int size, head, tail;
	void **queue;
};

struct Queue *mq_new(unsigned int cap);
void mq_destroy(struct Queue *q);
void mq_send(struct Queue *q, void *elm);
void *mq_recv(struct Queue *q);
int mq_recv_async(struct Queue *q, void ** result);

#endif /* MQ_H */
