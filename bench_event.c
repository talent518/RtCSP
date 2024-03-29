#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>

#include "bench.h"
#include "serialize.h"
#include "bench_event.h"
#include "socket.h"
#include "api.h"

#define max(a, b) ((a)>(b) ? (a) : (b))
#define min(a, b) ((a)<(b) ? (a) : (b))

typedef struct
{
	char *key;
	length_t keylen;
} srl_hash_t;

serialize_format_t hformat = SFT_STR(srl_hash_t, key, keylen, "parse receive hook key");

main_thread_t main_thread;
worker_thread_t *worker_threads;
volatile conn_send_recv_t *send_recv;

static pthread_mutex_t init_lock, conn_lock;
static pthread_cond_t init_cond;

static void signal_handler(const int fd, short event, void *arg);
static void timeout_req_handler(const int fd, short event, void *arg);

void send_request(conn_t *ptr) {
	GString gstr = {NULL, 0, 0};

	conn_info(ptr);
	
	serialize_string_ex(&gstr, send_recv, &hformat);

	if(send_recv->send_call(ptr, &gstr)) {
		socket_send(ptr, gstr.str, gstr.len);
	}

	free(gstr.str);
}

static void read_handler(int sock, short event, void *arg)
{
	volatile int data_len=0;
	volatile char *data=NULL;
	int ret;
	char chr='n';
	conn_t *ptr = (conn_t*) arg;
	worker_thread_t *me = &worker_threads[ptr->tid];

	conn_info(ptr);

	ret=socket_recv(ptr,&data,&data_len);
	if(ret<0) {//已放入缓冲区
	} else if(ret==0) {//关闭连接
		conn_info_ex(ptr,"close socket connect");
		socket_close(ptr);
		me->conns[ptr->index] = NULL;
		free(ptr);

		me->close_conn_num++;
		if(me->close_conn_num+me->complete_conn_num == me->conn_num) {
			me->run_time = microtime() - me->tmp_time;
			write(main_thread.write_fd, &chr, 1);
		}
	} else {//接收数据成功
		unsigned int tmplen;
		srl_hash_t hkey = {NULL, 0};
		
		tmplen = serialize_parse((void*)&hkey, &hformat, data, data_len);
		if(!tmplen) {
			fprintf(stderr, "Parse receive hook key failed(%s).\n", data);

			goto sendnext;
		}

		if(strcmp(send_recv->key, hkey.key)) {
			fprintf(stderr, "receive key not equal(%s != %s).\n", send_recv->key, hkey.key);

			goto sendnext;
		}

		ret = send_recv->recv_call(ptr, data+tmplen, data_len-tmplen);
		if(ret>0) {
			__sync_fetch_and_add(&main_thread.ok_requests, 1);
			me->ok_requests++;
		} else {
			fprintf(stderr, "receive data error(%s)\n", data+tmplen);
		}
sendnext:
		if(hkey.key) {
			free(hkey.key);
		}

		__sync_fetch_and_add(&main_thread.requests, 1);
		me->requests++;

		if((++ptr->requests) < bench_preconn_requests && !ptr->is_skip) {
			send_request(ptr);
		} else {
			ptr->is_skip = false;
			me->complete_conn_num++;
			if(me->close_conn_num+me->complete_conn_num == me->conn_num) {
				me->run_time = microtime() - me->tmp_time;
				write(main_thread.write_fd, &chr, 1);
			}
		}
	}
	if(data) {
		free(data);
	}
}

static void *worker_thread_handler(void *arg)
{
	conn_t *ptr;
	worker_thread_t *me = (worker_thread_t*)arg;
	me->tid = pthread_self();

	dprintf("thread %d created\n", me->id);

	pthread_mutex_lock(&init_lock);
	main_thread.nthreads++;
	pthread_cond_signal(&init_cond);
	pthread_mutex_unlock(&init_lock);

	event_base_loop(me->base, 0);

	event_del(&me->event);
	event_base_free(me->base);

	pthread_mutex_lock(&init_lock);
	main_thread.nthreads--;
	pthread_cond_signal(&init_cond);
	pthread_mutex_unlock(&init_lock);

	dprintf("thread %d exited\n", me->id);

	return NULL;
}

static void worker_notify_handler(const int fd, const short which, void *arg)
{
	register unsigned int buf_len,i;
	char chr;
	conn_t *ptr;
	worker_thread_t *me = arg;
	assert(fd == me->read_fd);

	buf_len = read(fd, &chr, 1);
	if (buf_len <= 0) {
		return;
	}

	dprintf("%s(%c)\n", __func__, chr);

	switch(chr) {
		case 'N':
			for(i=0; i<me->conn_num; i++) {
				ptr = me->conns[i];
				if(ptr) {
					ptr->is_skip = true;
				}
			}
			break;
		case 'n':
		case 'b': // begin send request
			me->requests = 0;
			me->ok_requests = 0;
			me->complete_conn_num = 0;
			if(me->close_conn_num == me->conn_num) {
				me->run_time = microtime() - me->tmp_time;
				write(main_thread.write_fd, &chr, 1);
				break;
			}
			me->tmp_time = microtime();
			for(i=0; i<me->conn_num; i++) {
				ptr = me->conns[i];
				if(ptr) {
					ptr->requests = 0;
					send_request(ptr);
				}
			}
			break;
		case 'c': // create connection
			me->conns = (conn_t**)malloc(sizeof(conn_t*)*bench_prethread_conns);
			me->conn_num = 0;

			dprintf("thread %d socket connectting...\n", me->id);
			me->tmp_time = microtime();
			for(i=0; i<bench_prethread_conns; i++) {
				int j = 0;
				do
				{
					j++;
					ptr = socket_connect(bench_host, bench_port);
				}
				while (!ptr && j<3);

				if(ptr) {
					ptr->tid = me->id;
					ptr->index = me->conn_num;
					
					event_set(&ptr->event, ptr->sockfd, EV_READ|EV_PERSIST, read_handler, ptr);
					event_base_set(me->base, &ptr->event);
					event_add(&ptr->event, NULL);

					me->conns[ptr->index] = ptr;
					me->conn_num++;

					__sync_fetch_and_add(&main_thread.conn_num, 1);
				}
			}
			me->conn_time = microtime() - me->tmp_time;
			dprintf("thread %d connect success(%d), fail(%d), runtime(%.3lfs)\n", me->id, me->conn_num, bench_prethread_conns - me->conn_num, me->conn_time);
			write(main_thread.write_fd, &chr, 1);
			break;
		case '-': // break thread
			me->run_time = microtime() - me->tmp_time;

			// close connection
			dprintf("thread %d socket closing...\n", me->id);
			me->tmp_time = microtime();
			for(i=0; i<me->conn_num; i++) {
				if(!me->conns[i]) {
					continue;
				}

				socket_close(me->conns[i]);

				free(me->conns[i]);
				me->conns[i] = NULL;
			}
			me->close_conn_time = microtime() - me->tmp_time;
			dprintf("thread %d socket closed runtime(%.3fs)\n", me->id, me->close_conn_time);

			free(me->conns);

			// break event loop
			event_base_loopbreak(me->base);
			break;
		default:
			break;
	}
}

static inline void send_recv_call() {
	register unsigned int i;

	send_recv->run_time = 0;
	send_recv->cause_close_conn_num = 0;
	send_recv->requests = 0;
	send_recv->ok_requests = 0;
	send_recv->throughput_requests = 0;
	send_recv->throughput_ok_requests = 0;
	for(i=0; i<bench_nthreads; i++) {
		send_recv->run_time = max(worker_threads[i].run_time,send_recv->run_time);
		send_recv->cause_close_conn_num += (worker_threads[i].close_conn_num - worker_threads[i].preclose_conn_num);
		send_recv->requests += worker_threads[i].requests;
		send_recv->ok_requests += worker_threads[i].ok_requests;
		send_recv->throughput_requests += (worker_threads[i].requests/worker_threads[i].run_time);
		send_recv->throughput_ok_requests += (worker_threads[i].ok_requests/worker_threads[i].run_time);
	}
	
	main_thread.requests = 0;
	main_thread.ok_requests = 0;

	main_thread.sum_requests = 0;
	main_thread.sum_ok_requests = 0;

	main_thread.sum_requests = 0;
	main_thread.sum_ok_requests = 0;

	main_thread.min_requests = UINT_MAX;
	main_thread.min_ok_requests = UINT_MAX;

	main_thread.max_requests = 0;
	main_thread.max_ok_requests = 0;

	main_thread.current_request_index = 0;
	memset(main_thread.second_requests, 0, sizeof(main_thread.second_requests));
	memset(main_thread.second_ok_requests, 0, sizeof(main_thread.second_ok_requests));
}

#ifdef HAVE_SKIP_BENCH
static void stdin_handler(const int fd, short event, void *arg) {
	register unsigned int i;
	char chr = 'N';
	char *buf = NULL;
	size_t len = 0;
	int ret;
	
	event_del(&main_thread.stdin_ev);
	main_thread.stdin_ev.ev_base = NULL;

	if((ret = getline(&buf, &len, stdin)) <= 0) {
		fprintf(stderr, "From stdin read a line error.\n");
		return;
	}

	for(i=0; i<bench_nthreads; i++) {
		write(worker_threads[i].write_fd, &chr, 1);
	}
}

void rebind_stdin_event(bool is_bind) {
	// stdin input event
	if(is_bind && main_thread.stdin_ev.ev_base) {
		return;
	}
	
	event_set(&main_thread.stdin_ev, STDIN_FILENO, EV_READ|EV_PERSIST, stdin_handler, NULL);
	event_base_set(main_thread.base, &main_thread.stdin_ev);
	if (event_add(&main_thread.stdin_ev, NULL) == -1) {
		fprintf(stderr, "bind stdin event\n");
		exit(1);
	}
}
#else
	#define rebind_stdin_event(e)
#endif

static void main_notify_handler(const int fd, const short which, void *arg)
{
	register unsigned int i;
	register int buf_len,c;
	char buf[64];
	assert(fd == main_thread.read_fd);

	buf_len = read(fd, buf, sizeof(buf));
	if (buf_len <= 0) {
		return;
	}

	for(c=0; c<buf_len; c++) {
		dprintf("%s(%c)\n", __func__, buf[c]);

		switch(buf[c]) {
			case 'n': // thread complete
				if((++main_thread.cthreads) < bench_nthreads) {
					break;
				} else {
					main_thread.cthreads = 0;
				}

				send_recv_call();

				if((++main_thread.send_recv_id) < bench_modules[main_thread.modid]->recvs_len) {
				} else if((++main_thread.modid) < bench_length) {
					main_thread.send_recv_id = 0;
				} else {
					send_recv = NULL;
					signal_handler(0, 0, NULL);
					return;
				}
			case 'b': // begin send request
				send_recv = &(bench_modules[main_thread.modid]->recvs[main_thread.send_recv_id]);
				send_recv->cause_close_conn_num = 0;
				send_recv->requests = 0;
				send_recv->ok_requests = 0;

				for(i=0; i<bench_nthreads; i++) {
					worker_threads[i].preclose_conn_num = worker_threads[i].close_conn_num;
					worker_threads[i].requests = 0;
					worker_threads[i].ok_requests = 0;
					write(worker_threads[i].write_fd, &buf[c], 1);
				}
				
				rebind_stdin_event(true);
				
				break;
			case 'c': // create connection for worker thread
				if((++main_thread.cthreads) < bench_nthreads) {
					break;
				}

				main_thread.run_time = microtime() - main_thread.tmp_time;
				printf("connection: total(%d), runtime(%.3f second), avg(%.3f/s)\n", main_thread.conn_num, main_thread.run_time, (double)(main_thread.conn_num)/main_thread.run_time);

				main_thread.cthreads = 0;
				main_thread.seconds = 0;
				buf[c] = 'b';
				c--;

				// rebind timeout event
				if (event_del(&main_thread.timeout_int) == -1) {
					fprintf(stderr, "Rebind timeout event error.\n");
					signal_handler(0, 0, NULL);
					return;
				}
				do {
					struct timeval tv;
					evutil_timerclear(&tv);
					tv.tv_sec = 1;
					event_set(&main_thread.timeout_int, -1, EV_PERSIST, timeout_req_handler, NULL);
					event_base_set(main_thread.base, &main_thread.timeout_int);
					if (event_add(&main_thread.timeout_int, &tv) == -1) {
						fprintf(stderr, "rebind timeout event\n");
						exit(1);
					}
					
					rebind_stdin_event(false);
				} while(0);
				break;
		}
	}
}

static void timeout_handler(const int fd, short event, void *arg) {
	main_thread.seconds++;
	printf("seconds(%06d), connections(%d), connection_avg(%.3f)\n", main_thread.seconds, main_thread.conn_num, (float)(main_thread.conn_num)/(float)(main_thread.seconds));
}

static void timeout_req_handler(const int fd, short event, void *arg) {
	unsigned int requests = __sync_lock_test_and_set(&main_thread.requests, 0);
	unsigned int ok_requests = __sync_lock_test_and_set(&main_thread.ok_requests, 0);
	float count = (main_thread.second_requests[main_thread.current_request_index] ? AVG_SECONDS : main_thread.current_request_index+1);

	main_thread.sum_requests -= main_thread.second_requests[main_thread.current_request_index];
	main_thread.sum_ok_requests -= main_thread.second_ok_requests[main_thread.current_request_index];

	main_thread.sum_requests += requests;
	main_thread.sum_ok_requests += ok_requests;

	main_thread.min_requests = min(main_thread.min_requests, requests);
	main_thread.min_ok_requests = min(main_thread.min_ok_requests, ok_requests);

	main_thread.max_requests = max(main_thread.max_requests, requests);
	main_thread.max_ok_requests = max(main_thread.max_ok_requests, ok_requests);

	printf("seconds(%06d), send_recv(%s), connections(%d), requests(%d): (%d/%d) min(%d/%d) max(%d/%d) avg(%.1f/%.1f)\n", main_thread.seconds++, send_recv->key, main_thread.conn_num, main_thread.current_request_index, ok_requests, requests, main_thread.min_ok_requests, main_thread.min_requests, main_thread.max_ok_requests, main_thread.max_requests, main_thread.sum_ok_requests/count, main_thread.sum_requests/count);

	main_thread.second_requests[main_thread.current_request_index] = requests;
	main_thread.second_ok_requests[main_thread.current_request_index] = ok_requests;
	main_thread.current_request_index = (main_thread.current_request_index+1)%AVG_SECONDS;
}

static void signal_handler(const int fd, short event, void *arg) {
	dprintf("%s: got signal %d\n", __func__, EVENT_SIGNAL(&main_thread.signal_int));

	event_del(&main_thread.signal_int);

	register unsigned int i;
	char chr = '-';
	for(i=0; i<bench_nthreads; i++) {
		dprintf("%s: notify thread exit %d\n", __func__, i);
		write(worker_threads[i].write_fd, &chr, 1);
	}

	for(i=0; i<bench_nthreads; i++) {
		dprintf("%s: wait thread exit %d\n", __func__, i);
		pthread_join(worker_threads[i].tid, NULL);
	}
	
	dprintf("%s: wait worker thread for condition lock\n", __func__);
	pthread_mutex_lock(&init_lock);
	while (main_thread.nthreads > 0) {
		pthread_cond_wait(&init_cond, &init_lock);
	}
	pthread_mutex_unlock(&init_lock);

	if(send_recv) {
		send_recv_call();
	}

	dprintf("%s: exit main thread\n", __func__);
	event_base_loopbreak(main_thread.base);
}

void worker_create(void *(*func)(void *), void *arg)
{
	pthread_t       thread;
	pthread_attr_t  attr;
	int             ret;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if ((ret = pthread_create(&thread, &attr, func, arg)) != 0) {
		fprintf(stderr, "Can't create thread: %s\n", strerror(ret));
		exit(1);
	}
	
	pthread_attr_destroy(&attr);
}

void thread_init() {
	pthread_mutex_init(&init_lock, NULL);
	pthread_cond_init(&init_cond, NULL);

	pthread_mutex_init(&conn_lock, NULL);

	worker_threads = calloc(bench_nthreads, sizeof(worker_thread_t));
	assert(worker_threads);

	register unsigned int i;
	int fds[2];
	for (i = 0; i < bench_nthreads; i++) {
        if (pipe(fds)) {
            fprintf(stderr, "Can't create notify pipe\n");
            exit(1);
        }

		worker_threads[i].id = i;
		worker_threads[i].read_fd = fds[0];
		worker_threads[i].write_fd = fds[1];

		worker_threads[i].base = event_init();
		if (worker_threads[i].base == NULL) {
			fprintf(stderr, "event_init()\n");
			exit(1);
		}

		event_set(&worker_threads[i].event, worker_threads[i].read_fd, EV_READ | EV_PERSIST, worker_notify_handler, &worker_threads[i]);
		event_base_set(worker_threads[i].base, &worker_threads[i].event);
		if (event_add(&worker_threads[i].event, 0) == -1) {
			fprintf(stderr, "event_add()\n");
			exit(1);
		}
	}

	for (i = 0; i < bench_nthreads; i++) {
		worker_create(worker_thread_handler, &worker_threads[i]);
	}

	/* Wait for all the worker_threads to set themselves up before returning. */
	pthread_mutex_lock(&init_lock);
	while (main_thread.nthreads < bench_nthreads) {
		pthread_cond_wait(&init_cond, &init_lock);
	}
	pthread_mutex_unlock(&init_lock);
}

static void print_test_info() {
	register unsigned int i,j;
	conn_send_recv_t *recv;

	for(i=0;i<bench_length;i++) {
		printf("%s:\n",bench_names[i]);
		for(j=0;j<bench_modules[i]->recvs_len;j++) {
			recv = &(bench_modules[i]->recvs[j]);
			recv->run_time = max(1, recv->run_time);
			printf("    %s: closes(%d), requests(%d / %d), runtime(%.1f second), ok_requests(%.1f/s), requests(%.1f/s)\n", recv->key, recv->cause_close_conn_num, recv->ok_requests, recv->requests, recv->run_time, recv->throughput_ok_requests, recv->throughput_requests);
		}
		printf("\n");
	}
}

void loop_event() {
	memset(&main_thread, 0, sizeof(main_thread_t));
	
	// init main thread
	main_thread.base = event_init();
	if (main_thread.base == NULL) {
		fprintf(stderr, "event_init( base )");
		exit(1);
	}
	main_thread.tid = pthread_self();

	int fds[2];
	if (pipe(fds)) {
		fprintf(stderr, "Can't create notify pipe");
		exit(1);
	}

	main_thread.read_fd = fds[0];
	main_thread.write_fd = fds[1];

	register unsigned int i;
	
#ifndef HAVE_SKIP_BENCH
	int n = -1;
	while(n<0 || n>=bench_length) {
		printf("Module list:\n");
		for(i=0; i<bench_length; i++) {
			printf("    %d) %s\n", i, bench_names[i]);
		}
		printf("Please input a number for module id: ");
		scanf("%d", &n);
	}
	main_thread.modid = n;
	n = -1;
	while(n<0 || n>=bench_modules[main_thread.modid]->recvs_len) {
		printf("Recv list for module \"%s\":\n", bench_names[main_thread.modid]);
		for(i=0; i<bench_modules[main_thread.modid]->recvs_len; i++) {
			printf("    %d) %s\n", i, bench_modules[main_thread.modid]->recvs[i].key);
		}
		printf("Please input a number for recv id: ");
		scanf("%d", &n);
	}
	main_thread.send_recv_id = n;
#else
	main_thread.modid = 0;
	main_thread.send_recv_id = 0;
#endif

	main_thread.cthreads = 0;
	main_thread.seconds = 0;
	
	for(i=0; i<AVG_SECONDS; i++) {
		main_thread.second_requests[i] = 0;
		main_thread.second_ok_requests[i] = 0;
	}
	main_thread.sum_requests = 0;
	main_thread.sum_ok_requests = 0;
	main_thread.min_requests = UINT_MAX;
	main_thread.min_ok_requests = UINT_MAX;
	main_thread.max_requests = 0;
	main_thread.max_ok_requests = 0;
	main_thread.requests = 0;
	main_thread.ok_requests = 0;
	main_thread.current_request_index = 0;

	// init notify thread
	thread_init();

	// listen notify event
	event_set(&main_thread.notify_ev, main_thread.read_fd, EV_READ | EV_PERSIST, main_notify_handler, NULL);
	event_base_set(main_thread.base, &main_thread.notify_ev);
	if (event_add(&main_thread.notify_ev, NULL) == -1) {
		fprintf(stderr, "main thread notify event");
		exit(1);
	}

	// timeout event
	struct timeval tv;
	evutil_timerclear(&tv);
	tv.tv_sec = 1;
	event_set(&main_thread.timeout_int, -1, EV_PERSIST, timeout_handler, NULL);
	event_base_set(main_thread.base, &main_thread.timeout_int);
	if (event_add(&main_thread.timeout_int, &tv) == -1) {
		fprintf(stderr, "timeout event");
		exit(1);
	}

	// int signal event
	event_set(&main_thread.signal_int, SIGINT, EV_SIGNAL|EV_PERSIST, signal_handler, NULL);
	event_base_set(main_thread.base, &main_thread.signal_int);
	if (event_add(&main_thread.signal_int, NULL) == -1) {
		fprintf(stderr, "int signal event");
		exit(1);
	}

	for(i=0;i<bench_length;i++) {
		if(bench_modules[i]->start) {
			bench_modules[i]->start();
		}
	}

	main_thread.tmp_time = microtime();
	char chr='c';
	for(i=0; i<bench_nthreads; i++) {
		write(worker_threads[i].write_fd, &chr, 1);
	}

	event_base_loop(main_thread.base, 0);

	for(i=0;i<bench_length;i++) {
		if(bench_modules[i]->stop) {
			bench_modules[i]->stop();
		}
	}

	event_del(&main_thread.notify_ev);
	event_del(&main_thread.timeout_int);
	event_del(&main_thread.signal_int);
#ifdef HAVE_SKIP_BENCH
	if(main_thread.stdin_ev.ev_base) {
		event_del(&main_thread.stdin_ev);
	}
#endif
	event_base_free(main_thread.base);

	print_test_info();

	free(worker_threads);
}
