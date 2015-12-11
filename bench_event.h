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

	unsigned int cthreads; // ��ɴ�����߳���
	unsigned int modid; // ģ��ID
	unsigned int send_recv_id; // ��������յľ��ID
	conn_send_recv_t *send_recv; // ��������յľ��ָ��

	volatile unsigned int conn_num; // ��ǰ������

	unsigned int second_requests[AVG_SECONDS], second_ok_requests[AVG_SECONDS]; // ���AVG_SECONDS���������/�ɹ����ļ�¼
	unsigned int current_request_index;

	unsigned int sum_requests, sum_ok_requests; // ���AVG_SECONDS���������/�ɹ�����ƽ��ֵ
	unsigned int min_requests, min_ok_requests; // ��Сֵ
	unsigned int max_requests, max_ok_requests; // ���ֵ

	volatile unsigned int requests,ok_requests; // ��ǰ���������/�ɹ�����ֵ
	unsigned int seconds;

	double tmp_time,run_time;
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

	unsigned int complete_conn_num; // ��ɵ�������
	unsigned int close_conn_num; // �رյ�������
	unsigned int preclose_conn_num; // ����ʧ����

	unsigned int requests,ok_requests;

	double tmp_time,run_time,conn_time,close_conn_time;
} worker_thread_t;

extern main_thread_t main_thread;
extern worker_thread_t *worker_threads;

void loop_event();

#endif