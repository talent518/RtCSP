#ifndef _LOOP_EVENT_H
#define _LOOP_EVENT_H

#include <stdbool.h>
#include <event.h>
#include <glib.h>
#include "queue.h"

typedef struct
{
	pthread_t tid;
	struct event_base *base;
	struct event notify_ev;
	struct event listen_ev;
	struct event signal_int;

	int nthreads;

	int sockfd;

	short ev_flags;
	int read_fd;
	int write_fd;
} listen_thread_t;

typedef struct
{
	pthread_t tid;
	struct event_base *base;
	struct event event;
	
	queue_t *accept_queue;
	queue_t *close_queue;

	short id;
	int read_fd;
	int write_fd;
} worker_thread_t;

extern listen_thread_t listen_thread;
extern worker_thread_t *worker_threads;

void is_accept_conn(bool do_accept);
void loop_event (int sockfd);

#endif
