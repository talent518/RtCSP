#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <glib.h>

#include "api.h"
#include "RtCSP.h"
#include "conn.h"
#include "loop_event.h"
#include "serialize.h"

static pthread_mutex_t init_lock, conn_lock;
static pthread_cond_t init_cond;

int rtcsp_nthreads;

listen_thread_t listen_thread;
worker_thread_t *worker_threads;

GHashTable *ht_conn_recvs = NULL;

typedef struct
{
	char *key;
	unsigned int keylen;
} srl_hash_t;

static serialize_format_t hformat = SFT_STR(srl_hash_t, key, keylen, "parse receive hook key");

inline void hook_conn_close(conn_t *ptr) {
	int i;
	for(i=0;i<rtcsp_length;i++) {
		if(rtcsp_modules[i]->conn_close) {
			rtcsp_modules[i]->conn_close(ptr);
		}
	}
}

static void listen_handler(const int fd, const short which, void *arg);
bool update_accept_event(short new_flags) {
	if(listen_thread.ev_flags==new_flags) {
		return false;
	}

    if (event_del(&listen_thread.listen_ev) == -1) {
		return false;
	}

	listen_thread.ev_flags=new_flags;

	event_set(&listen_thread.listen_ev, listen_thread.sockfd, new_flags, listen_handler, NULL);
	event_base_set(listen_thread.base, &listen_thread.listen_ev);
    event_add(&listen_thread.listen_ev, NULL);

	return true;
}

void is_accept_conn_ex(bool do_accept) {
	if (do_accept) {
		if (update_accept_event(EV_READ | EV_PERSIST) && listen(listen_thread.sockfd, rtcsp_backlog) != 0) {
			perror("listen");
		}
	} else {
		if (update_accept_event(0) && listen(listen_thread.sockfd, 0) != 0) {
			perror("listen");
		}
	}
}

void is_accept_conn(bool do_accept) {
	char chr;
	if (do_accept) {
		chr='e'; // enable
	} else {
		chr='d'; // disable
	}

	write(listen_thread.write_fd, &chr, 1);
}

static void *worker_thread_handler(void *arg)
{
	worker_thread_t *me = arg;
	me->tid = pthread_self();

	dprintf("thread %d created\n", me->id);

	unsigned int i;
	for(i=0;i<rtcsp_length;i++) {
		if(rtcsp_modules[i]->thread_init) {
			rtcsp_modules[i]->thread_init(me);
		}
	}

    pthread_mutex_lock(&init_lock);
    listen_thread.nthreads++;
	pthread_cond_signal(&init_cond);
    pthread_mutex_unlock(&init_lock);

	event_base_loop(me->base, 0);

	for(i=0;i<rtcsp_length;i++) {
		if(rtcsp_modules[i]->thread_destory) {
			rtcsp_modules[i]->thread_destory(me);
		}
	}

	event_del(&me->event);
	event_base_free(me->base);

    pthread_mutex_lock(&init_lock);
    listen_thread.nthreads--;
    pthread_cond_signal(&init_cond);
    pthread_mutex_unlock(&init_lock);

	dprintf("thread %d exited\n", me->id);

	pthread_detach(me->tid);
	pthread_exit(NULL);

	return NULL;
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

static void read_handler(int sock, short event,	void* arg)
{
	volatile int data_len=0;
	volatile char *data=NULL;
	int ret;
	conn_t *ptr = (conn_t *) arg;

	conn_info(ptr);

	ret=socket_recv(ptr,&data,&data_len);
	if(ret<0) {//已放入缓冲区
	} else if(ret==0) {//关闭连接
		event_del(&ptr->event);
		hook_conn_close(ptr);
		remove_conn(ptr);

		is_accept_conn(true);
	} else {//接收数据成功
		conn_recv_func_t call;
		unsigned int tmplen;
		srl_hash_t hkey = {NULL, 0};
		
		tmplen = serialize_parse((void*)&hkey, &hformat, data, data_len);
		if(!tmplen) {
			fprintf(stderr, "Parse receive hook key failed(%s).\n", data);
			if(hkey.key) {
				free(hkey.key);
			}
			goto end_read;
		}

		call = g_hash_table_lookup(ht_conn_recvs,hkey.key);
		if(!call) {
			fprintf(stderr, "Connection receive hook (%s) not exists.\n", hkey.key);
			free(hkey.key);
			goto end_read;
		}

		GString gstr = {NULL,0,0};

		g_string_append_len(&gstr, data, tmplen);

		if(call(ptr, data+tmplen, data_len-tmplen, &gstr)) {
			socket_send(ptr, gstr.str, gstr.len);
		}

		free(hkey.key);
		free(gstr.str);
	}
end_read:
	if(data) {
		free(data);
	}
}

static void notify_handler(const int fd, const short which, void *arg)
{
	int buffer_len,i;
	char chr,buffer[1024];
	worker_thread_t *me = arg;
	conn_t *ptr;

	if (fd != me->read_fd) {
		fprintf(stderr, "notify_handler error : fd != me->read_fd\n");
		exit(1);
	}

	buffer_len = read(fd, buffer, sizeof(buffer));
	if (buffer_len <= 0) {
		return;
	}

	for(i=0;i<buffer_len;i++) {
		chr=buffer[i];

		dprintf("notify_handler: notify(%c) threadId(%d)\n", chr, me->id);

		switch(chr) {
			case 'x': // 处理连接关闭对列
				ptr=queue_pop(me->close_queue);

				assert(ptr);

				conn_info(ptr);
				clean_conn(ptr);
				hook_conn_close(ptr);
				remove_conn(ptr);

				is_accept_conn(true);
				break;
			case '-': // 结束worker/notify线程
				event_base_loopbreak(me->base);
				break;
			case 'l':
				ptr=queue_pop(me->accept_queue);

				assert(ptr);
				assert(ptr->thread == me);

				conn_info(ptr);

				event_set(&ptr->event, ptr->sockfd, EV_READ|EV_PERSIST, read_handler, ptr);
				event_base_set(me->base, &ptr->event);
				event_add(&ptr->event, NULL);
				break;
			default:
				break;
		}
	}
}

void thread_init() {
	pthread_mutex_init(&init_lock, NULL);
    pthread_cond_init(&init_cond, NULL);

	pthread_mutex_init(&conn_lock, NULL);

	worker_threads = calloc(rtcsp_nthreads, sizeof(worker_thread_t));
	assert(worker_threads);

	int i;
	int fds[2];
	for (i = 0; i < rtcsp_nthreads; i++) {
        if (pipe(fds)) {
            perror("Can't create notify pipe");
            exit(1);
        }

		worker_threads[i].id = i;
		worker_threads[i].read_fd = fds[0];
		worker_threads[i].write_fd = fds[1];

		worker_threads[i].base = event_init();
		if (worker_threads[i].base == NULL) {
			perror("event_init()");
			exit(1);
		}

		event_set(&worker_threads[i].event, worker_threads[i].read_fd, EV_READ | EV_PERSIST, notify_handler, &worker_threads[i]);
		event_base_set(worker_threads[i].base, &worker_threads[i].event);
		if (event_add(&worker_threads[i].event, 0) == -1) {
			perror("event_add()");
			exit(1);
		}

		worker_threads[i].accept_queue=queue_init();
		worker_threads[i].close_queue=queue_init();
	}

	for (i = 0; i < rtcsp_nthreads; i++) {
		worker_create(worker_thread_handler, &worker_threads[i]);
	}

	/* Wait for all the worker_threads to set themselves up before returning. */
    pthread_mutex_lock(&init_lock);
    while (listen_thread.nthreads < rtcsp_nthreads) {
        pthread_cond_wait(&init_cond, &init_lock);
    }
    pthread_mutex_unlock(&init_lock);
}

static void listen_notify_handler(const int fd, const short which, void *arg)
{
	int buf_len,i;
	char buf[1024];

	assert(fd == listen_thread.read_fd);

	buf_len = read(fd, buf, 1024);
	if (buf_len <= 0) {
		return;
	}

	for(i=0; i<buf_len; i++) {
		dprintf("%s(%c)\n", __func__, buf[i]);

		switch(buf[i]) {
			case 'e': // 接受连接(enable)
				is_accept_conn_ex(true);
				break;
			case 'd': // 禁止连接(disable)
				is_accept_conn_ex(false);
				break;
			default:
				break;
		}
	}
}

static void listen_handler(const int fd, const short which, void *arg)
{
	unsigned int i;
	int conn_fd, ret;
	struct sockaddr_in pin;
	socklen_t len = sizeof(pin);
	conn_fd = accept(fd, (struct sockaddr *)&pin, &len);
	
	if(conn_fd <= 0) {
		return;
	}

	int send_timeout = 1000, recv_timeout = 1000;
	setsockopt(conn_fd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(int));//发送超时
	setsockopt(conn_fd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(int));//接收超时

	conn_t *ptr = (conn_t *)malloc(sizeof(conn_t));

	bzero(ptr, sizeof(conn_t));

	ptr->sockfd = conn_fd;
	inet_ntop(AF_INET, &pin.sin_addr, ptr->host, sizeof(ptr->host));
	ptr->port = ntohs(pin.sin_port);

	if(CONN_NUM >= rtcsp_maxclients) {
		is_accept_conn_ex(false);

		conn_info(ptr);

		for(i=0;i<rtcsp_length;i++) {
			if(rtcsp_modules[i]->conn_denied) {
				rtcsp_modules[i]->conn_denied(ptr);
			}
		}

		clean_conn(ptr);

		free(ptr);
	} else {
		insert_conn(ptr);

		worker_thread_t *thread = worker_threads + (ptr->index-1) % rtcsp_nthreads;

		ptr->thread = thread;

		bool flag = true;
		for(i=0;i<rtcsp_length;i++) {
			if(rtcsp_modules[i]->conn_accept && !rtcsp_modules[i]->conn_accept(ptr)) {
				flag = false;
				break;
			}
		}

		if(flag) {
			queue_push(thread->accept_queue, ptr);

			conn_info(ptr);

			char chr='l';
			write(thread->write_fd, &chr, 1);
		} else {
			conn_info(ptr);
			clean_conn(ptr);
			remove_conn(ptr);
		}
	}
}

static void signal_handler(const int fd, short event, void *arg) {
	dprintf("%s: got signal %d\n", __func__, EVENT_SIGNAL(&listen_thread.signal_int));

	event_del(&listen_thread.signal_int);

	is_accept_conn_ex(false);

	unsigned int i;
	char chr = '-';
	for(i=0; i<rtcsp_nthreads; i++) {
		dprintf("%s: notify thread exit %d\n", __func__, i);
		write(worker_threads[i].write_fd, &chr, 1);
	}

	dprintf("%s: wait worker thread\n", __func__);
    pthread_mutex_lock(&init_lock);
    while (listen_thread.nthreads > 0) {
        pthread_cond_wait(&init_cond, &init_lock);
    }
    pthread_mutex_unlock(&init_lock);

	dprintf("%s: close conn\n", __func__);

	conn_t *ptr;

	for(i=0; i<rtcsp_maxclients; i++) {
		ptr = iconns[i];

		if(!ptr) {
			continue;
		}

		conn_info(ptr);
		
		clean_conn(ptr);

		hook_conn_close(ptr);
	}

	dprintf("%s: exit main thread\n", __func__);
	event_base_loopbreak(listen_thread.base);
}

void loop_event (int sockfd) {
	// init main thread
	listen_thread.sockfd = sockfd;
	listen_thread.base = event_init();
	if (listen_thread.base == NULL) {
		perror("event_init( base )");
		exit(1);
	}
	listen_thread.tid = pthread_self();
	listen_thread.nthreads = 0;

	int fds[2];
	if (pipe(fds)) {
		perror("Can't create notify pipe");
		exit(1);
	}

	listen_thread.read_fd = fds[0];
	listen_thread.write_fd = fds[1];

	// init notify thread
	thread_init();

	// listen notify event
	event_set(&listen_thread.notify_ev, listen_thread.read_fd, EV_READ | EV_PERSIST, listen_notify_handler, NULL);
	event_base_set(listen_thread.base, &listen_thread.notify_ev);
	if (event_add(&listen_thread.notify_ev, NULL) == -1) {
		perror("event_add()");
		exit(1);
	}

	// listen event
	listen_thread.ev_flags=EV_READ | EV_PERSIST;
	event_set(&listen_thread.listen_ev, sockfd, listen_thread.ev_flags, listen_handler, NULL);
	event_base_set(listen_thread.base, &listen_thread.listen_ev);
	if (event_add(&listen_thread.listen_ev, NULL) == -1) {
		perror("listen event");
		exit(1);
	}

	// int signal event
	event_set(&listen_thread.signal_int, SIGINT, EV_SIGNAL|EV_PERSIST, signal_handler, NULL);
	event_base_set(listen_thread.base, &listen_thread.signal_int);
	if (event_add(&listen_thread.signal_int, NULL) == -1) {
		perror("int signal event");
		exit(1);
	}

	attach_conn();

	unsigned int i,j;
	conn_recv_t *ptr;
	ht_conn_recvs = g_hash_table_new(g_str_hash, g_str_equal);
	for(i=0;i<rtcsp_length;i++) {
		for(j=0;j<rtcsp_modules[i]->conn_recv_len;j++) {
			ptr = &(rtcsp_modules[i]->conn_recvs[j]);
			if(g_hash_table_lookup(ht_conn_recvs,ptr->key)) {
				fprintf(stderr, "Connection receive hook (%s->%s) is exists.\n", rtcsp_names[i], ptr->key);
			} else {
				g_hash_table_insert(ht_conn_recvs,ptr->key,ptr->call);
			}
		}
		if(rtcsp_modules[i]->start) {
			rtcsp_modules[i]->start();
		}
	}

	event_base_loop(listen_thread.base, 0);

	event_del(&listen_thread.notify_ev);
	event_del(&listen_thread.listen_ev);
	event_del(&listen_thread.signal_int);
	event_base_free(listen_thread.base);

	free(worker_threads);

	for(i=0;i<rtcsp_length;i++) {
		if(rtcsp_modules[i]->stop) {
			rtcsp_modules[i]->stop();
		}
	}

	g_hash_table_destroy(ht_conn_recvs);

	shutdown(sockfd, 2);
	close(sockfd);

	detach_conn();
}
