#ifndef _BENCH_H
#define _BENCH_H

#include <stdlib.h>
#include <stdbool.h>
#include <event.h>
#include <glib.h>

#define rtcsp_maxrecvs bench_maxrecvs

#ifdef DEBUG
	#define dprintf(...) fprintf(stdout,__VA_ARGS__)
#else
	#define dprintf(...)
#endif

#ifdef DEBUG_INFO
	#define _conn_info_ex(fd,ptr,append) fprintf(fd,append" in %20s on line (%3d): thread(%3d) sockfd(%5d)!\n", __func__, __LINE__, ptr->tid, ptr->sockfd)
#else
	#define _conn_info_ex(fd,ptr,append)
#endif

#define conn_info(ptr) _conn_info_ex(stdout,ptr,"[ conn_info ] ")
#define conn_info_ex(ptr,append) _conn_info_ex(stderr,ptr,append)

typedef struct _conn_t{
	unsigned int tid;

	int index;
	
	int sockfd;
	char host[16];
	int port;

	char *rbuf;
	int rbytes;
	int rsize;

	struct event event;
} conn_t;

typedef bool (*conn_send_func_t)(conn_t *, GString *);
typedef bool (*conn_recv_func_t)(conn_t *, const char *, int);

typedef struct
{
	char *key;
	unsigned int keylen;
	conn_send_func_t send_call;
	conn_recv_func_t recv_call;
} conn_send_recv_t;

typedef struct
{
	conn_send_recv_t *recvs;
	unsigned int recvs_len;
} bench_module_t;

extern char *bench_host;
extern unsigned int bench_port;
extern unsigned int bench_nthreads;
extern unsigned int bench_requests;
extern int bench_maxrecvs;

extern unsigned int bench_length;
extern char *bench_names[];
extern bench_module_t *bench_modules[];

#endif