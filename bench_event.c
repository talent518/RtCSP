#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

#include "bench.h"
#include "serialize.h"
#include "bench_event.h"
#include "socket.h"

typedef struct
{
	char *key;
	unsigned int keylen;
} srl_hash_t;

serialize_format_t hformat = SFT_STR(srl_hash_t, key, keylen, "parse receive hook key");

main_thread_t main_thread;
worker_thread_t *worker_threads;
volatile conn_send_recv_t *send_recv;

static pthread_mutex_t init_lock, conn_lock;
static pthread_cond_t init_cond;

static void signal_handler(const int fd, short event, void *arg);

void send_request(conn_t *ptr) {
	srl_hash_t hkey = {NULL, 0};
	volatile char *buffer = NULL;
	unsigned int buflen;
	volatile char *data = NULL;
	int data_len;

	printf("%s(%d:%d)(%s)\n", __func__, __LINE__, ptr->tid, data);
	data_len = send_recv->send_call(ptr, (volatile char **)&data);
	printf("%s(%d:%d)(%s)\n", __func__, __LINE__, ptr->tid, data);
	if(data_len>0 && data) {
		hkey.key = send_recv->key;
		hkey.keylen = send_recv->keylen;

		printf("%s(%d:%d)(%s)\n", __func__, __LINE__, ptr->tid, data);

		buflen = serialize_string(&hkey, &hformat, (volatile char **)&buffer);
		if(buflen > 0) {
			buffer = (char*)realloc((void*)buffer, buflen+data_len+1);

			memcpy((void*)(buffer+buflen), (void*)data, data_len);
			memset((void*)(buffer+buflen+data_len), 0, 1);

			socket_send(ptr, (char*)buffer, buflen+data_len);
		}
		printf("%s(%d:%d)(%s)\n", __func__, __LINE__, ptr->tid, data);

		if(buffer) {
			free((void*)buffer);
		}
	}
	printf("%s(%d:%d)(%s)\n", __func__, __LINE__, ptr->tid, data);

	if(data) {
		free((void*)data);
	}
	printf("%s(%d:%d)(%s)\n", __func__, __LINE__, ptr->tid, data);
}

static void read_handler(int sock, short event, void *arg)
{
	int data_len=0,ret;
	char *data=NULL;
	conn_t *ptr = (conn_t*) arg;
	worker_thread_t *me = &worker_threads[ptr->tid];

	conn_info(ptr);

	ret=socket_recv(ptr,&data,&data_len);
	if(ret<0) {//�ѷ��뻺����
	} else if(ret==0) {//�ر�����
		conn_info_ex(ptr,"close socket connect");
		socket_close(ptr);
		me->conns[ptr->index] = NULL;
		free(ptr);
	} else {//�������ݳɹ�
		unsigned int tmplen;
		srl_hash_t hkey = {NULL, 0};
		
		tmplen = serialize_parse((void*)&hkey, &hformat, data, data_len);
		if(!tmplen) {
			printf("Parse receive hook key failed(%s).\n", data);

			goto sendnext;
		}

		if(strcmp(send_recv->key, hkey.key)) {
			printf("receive key not equal(%s != %s).\n", send_recv->key, hkey.key);

			goto sendnext;
		}

		ret = send_recv->recv_call(ptr, data+tmplen, data_len-tmplen);
		if(ret>0) {
			// TODO recv OK
		} else {
			printf("receive data error(%s)\n", data+tmplen);
		}
sendnext:
		if(hkey.key) {
			free(hkey.key);
		}

		send_request(ptr);
	}
	if(data) {
		free(data);
	}
}

static void *worker_thread_handler(void *arg)
{
	unsigned int i,j;
	conn_t *ptr;
	worker_thread_t *me = (worker_thread_t*)arg;
	me->tid = pthread_self();

	dprintf("thread %d created\n", me->id);

	me->conns = (conn_t**)malloc(sizeof(conn_t*)*bench_requests);
	me->conn_num = 0;

	dprintf("thread %d socket connectting...\n", me->id);
	for(i=0; i<bench_requests; i++) {
		j = 0;
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
		}
	}

	dprintf("thread %d connect success(%d), fail(%d)\n", me->id, me->conn_num, bench_requests - me->conn_num);

    pthread_mutex_lock(&init_lock);

    main_thread.nthreads++;
	pthread_cond_signal(&init_cond);
    pthread_mutex_unlock(&init_lock);

	event_base_loop(me->base, 0);
	
	dprintf("thread %d socket closing...\n", me->id);
	for(i=0; i<me->conn_num; i++) {
		if(!me->conns[i]) {
			continue;
		}

		socket_close(me->conns[i]);

		free(me->conns[i]);
		me->conns[i] = NULL;
	}
	dprintf("thread %d socket closed\n", me->id);

	dprintf("thread %d exited\n", me->id);
    pthread_mutex_lock(&init_lock);

    main_thread.nthreads--;
    pthread_cond_signal(&init_cond);
    pthread_mutex_unlock(&init_lock);

	pthread_detach(me->tid);
	pthread_exit(NULL);

	return NULL;
}

static void worker_notify_handler(const int fd, const short which, void *arg)
{
	unsigned int buf_len,i;
	char chr[1];
	worker_thread_t *me = arg;
	assert(fd == me->read_fd);

	buf_len = read(fd, chr, 1);
	if (buf_len <= 0) {
		return;
	}

	switch(chr[0]) {
		case 'n':
		case 'b': // begin send request
			for(i=0; i<me->conn_num; i++) {
				if(me->conns[i]) {
					send_request(me->conns[i]);
				}
			}
			break;
		case '-':
			event_base_loopbreak(me->base);
			break;
		default:
			break;
	}
}

static void main_notify_handler(const int fd, const short which, void *arg)
{
	unsigned int buf_len,i;
	char chr[1];
	assert(fd == main_thread.read_fd);

	buf_len = read(fd, chr, 1);
	if (buf_len <= 0) {
		return;
	}

	switch(chr[0]) {
		case 'n': // thread complete
			if((++main_thread.cthreads) < bench_nthreads) {
				return;
			} else {
				main_thread.cthreads = 0;
			}

			if(main_thread.send_recv_id + 1 < bench_modules[main_thread.modid]->recvs_len) {
				main_thread.send_recv_id++;
			} else if(main_thread.modid + 1 < bench_length) {
				main_thread.modid++;
				main_thread.send_recv_id = 0;
			} else {
				signal_handler(0, 0, NULL);
				return;
			}
		case 'b': // begin send request
			send_recv = &(bench_modules[main_thread.modid]->recvs[main_thread.send_recv_id]);

			for(i=0; i<bench_nthreads; i++) {
				write(worker_threads[i].write_fd, &chr, 1);
			}
			break;
		default:
			break;
	}
}

static void signal_handler(const int fd, short event, void *arg) {
	dprintf("%s: got signal %d\n", __func__, EVENT_SIGNAL(&main_thread.signal_int));

	event_del(&main_thread.signal_int);

	unsigned int i;
	char chr = '-';
	for(i=0; i<bench_nthreads; i++) {
		dprintf("%s: notify thread exit %d\n", __func__, i);
		write(worker_threads[i].write_fd, &chr, 1);
	}

	dprintf("%s: wait worker thread %d\n", __func__);
    pthread_mutex_lock(&init_lock);
    while (main_thread.nthreads > 0) {
        pthread_cond_wait(&init_cond, &init_lock);
    }
    pthread_mutex_unlock(&init_lock);

	dprintf("%s: exit main thread\n", __func__);
	event_base_loopbreak(main_thread.base);
}

void worker_create(void *(*func)(void *), void *arg)
{
	pthread_t       thread;
	pthread_attr_t  attr;
	int             ret;

	pthread_attr_init(&attr);

	if ((ret = pthread_create(&thread, &attr, func, arg)) != 0) {
		fprintf(stderr, "Can't create thread: %s\n", strerror(ret));
		exit(1);
	}
}

void thread_init() {
	pthread_mutex_init(&init_lock, NULL);
    pthread_cond_init(&init_cond, NULL);

	pthread_mutex_init(&conn_lock, NULL);

	worker_threads = calloc(bench_nthreads, sizeof(worker_thread_t));
	assert(worker_threads);

	unsigned int i;
	int fds[2];
	for (i = 0; i < bench_nthreads; i++) {
        if (pipe(fds)) {
            fprintf(stderr, "Can't create notify pipe");
            exit(1);
        }

		worker_threads[i].id = i;
		worker_threads[i].read_fd = fds[0];
		worker_threads[i].write_fd = fds[1];

		worker_threads[i].base = event_init();
		if (worker_threads[i].base == NULL) {
			fprintf(stderr, "event_init()");
			exit(1);
		}

		event_set(&worker_threads[i].event, worker_threads[i].read_fd, EV_READ | EV_PERSIST, worker_notify_handler, &worker_threads[i]);
		event_base_set(worker_threads[i].base, &worker_threads[i].event);
		if (event_add(&worker_threads[i].event, 0) == -1) {
			fprintf(stderr, "event_add()");
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

void loop_event() {
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

	main_thread.modid = 0;
	main_thread.send_recv_id = 0;
	main_thread.cthreads = 0;

	// init notify thread
	thread_init();

	// listen notify event
	event_set(&main_thread.notify_ev, main_thread.read_fd, EV_READ | EV_PERSIST, main_notify_handler, NULL);
	event_base_set(main_thread.base, &main_thread.notify_ev);
	if (event_add(&main_thread.notify_ev, NULL) == -1) {
		fprintf(stderr, "event_add()");
		exit(1);
	}

	// int signal event
	event_set(&main_thread.signal_int, SIGINT, EV_SIGNAL|EV_PERSIST, signal_handler, NULL);
	event_base_set(main_thread.base, &main_thread.signal_int);
	if (event_add(&main_thread.signal_int, NULL) == -1) {
		fprintf(stderr, "int signal event");
		exit(1);
	}

	char chr[1] = {'b'};
	write(main_thread.write_fd, chr, 1);

	event_base_loop(main_thread.base, 0);
}