#ifndef _SERVER_H
#define _SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "config.h"
#include "conn.h"
#include "api.h"

#ifdef DEBUG
	#define INIT_RUNTIME() double runtime=microtime(),tmpruntime
	#define INFO_RUNTIME(info) tmpruntime=microtime();printf("[ " info " ] %20s: %.3fs\n", __func__, tmpruntime-runtime);runtime=tmpruntime
	#define rprintf(...) printf(__VA_ARGS__)
#else
	#define INIT_RUNTIME()
	#define INFO_RUNTIME(info)
	#define rprintf(...)
#endif

#ifdef DEBUG
#define dprintf(...) fprintf(stdout,__VA_ARGS__)
#define conn_info(ptr) _conn_info_ex(stdout,ptr,"[ conn_info ] ")
#else
#define dprintf(...)
#define conn_info(ptr)
#endif

#define _conn_info(ptr) _conn_info_ex(stdout,ptr,"[ _conn_info ] ")

#define _conn_info_ex(fd,ptr,append) fprintf(fd,append" in %20s on line (%3d): ref_count(%5d), index(%5d), sockfd(%5d), host(%15s), port(%5d)!\n", __func__, __LINE__, ptr->ref_count, ptr->index, ptr->sockfd, ptr->host, ptr->port)
#define conn_info_ex(ptr,append) _conn_info_ex(stderr,ptr,append)

typedef void (*server_func_t)(void);
typedef void (*thread_func_t)(worker_thread_t*);
typedef void (*conn_func_t)(conn_t*);
typedef char *(*conn_recv_func_t)(conn_t*,const char *, int);

typedef struct
{
	char *key;
	conn_recv_func_t call;
} conn_recv_t;
typedef struct
{
	server_func_t start,stop;
	thread_func_t thread_init,thread_destory;
	conn_func_t conn_accept,conn_denied,conn_close;
	conn_recv_t *conn_recv;
	size_t conn_recv_len;
} rtcsp_module_t;

extern volatile char *rtcsp_optarg;
extern volatile int rtcsp_optind;

extern unsigned int rtcsp_backlog;

extern char *rtcsp_host;
extern short int rtcsp_port;
extern char *rtcsp_pidfile;

extern char *rtcsp_user;
extern int rtcsp_nthreads;
extern int rtcsp_maxclients;
extern int rtcsp_maxrecvs;

extern int rtcsp_length;
extern char *rtcsp_names[];
extern rtcsp_module_t *rtcsp_modules[];

#endif
