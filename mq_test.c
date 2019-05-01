#include <stdio.h>
#include <pthread.h>
#include "mq.h"

void *thrd_func(void *q)
{
	struct Queue *mq = *(struct Queue **)q;
	int *p;
	for (int i = 0; i < 10000; i++)
	{
		if (mq_recv_async(mq, (void **)&p) == 0)
		{
			printf("%d ", *p);
		}
	}
	putchar('\n');

	return NULL;
}

int main()
{
	struct Queue *q = mq_new(3);
	pthread_t thrd;
	int a = 1;
	int b = 2;
	int c = 3;
	int d = 4;
	int e = 5;

	mq_send(q, &a);
	mq_send(q, &b);
	mq_send(q, &c);
	pthread_create(&thrd, NULL, &thrd_func, &q);
	mq_send(q, &d);
	mq_send(q, &e);

	pthread_join(thrd, NULL);

	mq_destroy(q);

	pthread_exit(NULL);
}
