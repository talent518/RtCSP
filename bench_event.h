#ifndef _BENCH_EVENT_H
#define _BENCH_EVENT_H

#include <stdbool.h>
#include <pthread.h>
#include <event.h>
#include <glib.h>
#include "bench.h"

#define AVG_SECONDS 10

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

	unsigned int second_requests[AVG_SECONDS], second_ok_requests[AVG_SECONDS]; // 最近AVG_SECONDS秒的请求数/成功数的记录
	unsigned int current_request_index;

	unsigned int sum_requests, sum_ok_requests; // 最近AVG_SECONDS秒的请求数/成功数的平均值
	unsigned int min_requests, min_ok_requests; // 最小值
	unsigned int max_requests, max_ok_requests; // 最大值

	unsigned int requests,ok_requests; // 当前秒的请求数/成功数的值
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