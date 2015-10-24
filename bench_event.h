#ifndef _BENCH_EVENT_H
#define _BENCH_EVENT_H

#include <stdbool.h>
#include <pthread.h>
#include <event.h>
#include "bench.h"

typedef struct
{
	pthread_t tid;
	struct event_base *base;
	struct event notify_ev;
	struct event timeout_int;
	struct event signal_int;

	unsigned int nthreads;

	int read_fd;
	int write_fd;

	unsigned int cthreads;
	unsigned int modid;
	unsigned int send_recv_id;
	conn_send_recv_t *send_recv;

	unsigned int requests;
	unsigned int ok_requests;
} main_thread_t;

typedef struct
{
	pthread_t tid;
	struct event_base *base;
	struct event event;

	unsigned int id;
	int read_fd;
	int write_fd;

	conn_t **conns;
	unsigned int conn_num;
} worker_thread_t;

extern main_thread_t main_thread;
extern worker_thread_t *worker_threads;

void loop_event();

#endif