#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "mq.h"

struct Queue *mq_new(unsigned int cap)
{
	if (cap < 1u) {
		fprintf(stderr, "%s: capacity must be positive\n", __func__);
		exit(EXIT_FAILURE);
	}

	struct Queue *q = (struct Queue*)malloc(sizeof(*q));
	if (!q) {
		perror(__func__);
		exit(EXIT_FAILURE);
	}

	q->queue = (void**)malloc(sizeof(q->queue[0]) * (cap + 1));
	if (!q->queue) {
		perror(__func__);
		exit(EXIT_FAILURE);
	}

	q->size = cap + 1u;
	q->head = q->tail = 0;

	pthread_mutex_init(&q->lock, NULL);
	pthread_cond_init(&q->wait_room, NULL);
	pthread_cond_init(&q->wait_data, NULL);

	return q;
}

void mq_destroy(struct Queue *q)
{
	free(q->queue);
	free(q);
}

int mq_recv_async(struct Queue *q, void ** result)
{
	pthread_mutex_lock(&q->lock);
	while (q->head == q->tail) {
		pthread_mutex_unlock(&q->lock);
		return -1;
	}

	*result = q->queue[q->tail];
	q->queue[q->tail] = NULL;
	q->tail = (q->tail + 1u) % q->size;

	pthread_cond_signal(&q->wait_room);

	pthread_mutex_unlock(&q->lock);

	return 0;
}

void *mq_recv(struct Queue *q)
{
	void *elm;

	pthread_mutex_lock(&q->lock);
	while (q->head == q->tail) {
		pthread_cond_wait(&q->wait_data, &q->lock);
	}

	elm = q->queue[q->tail];
	q->queue[q->tail] = NULL;
	q->tail = (q->tail + 1u) % q->size;

	pthread_cond_signal(&q->wait_room);

	pthread_mutex_unlock(&q->lock);

	return elm;
}

void mq_send(struct Queue *q, void *elm)
{
	pthread_mutex_lock(&q->lock);
	while (((q->head + 1u) % q->size) == q->tail) {
		pthread_cond_wait(&q->wait_room, &q->lock);
	}

	q->queue[q->head] = elm;
	q->head = (q->head + 1u) % q->size;

	pthread_cond_signal(&q->wait_data);
	pthread_mutex_unlock(&q->lock);
}
