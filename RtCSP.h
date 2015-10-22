#ifndef _SERVER_H
#define _SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <glib.h>

#include "config.h"
#include "conn.h"
#include "api.h"

#ifdef DEBUG
	#define dprintf(...) fprintf(stdout,__VA_ARGS__)
#else
	#define dprintf(...)
#endif

#ifdef DEBUG_INFO
	#define _conn_info_ex(fd,ptr,append) fprintf(fd,append" in %20s on line (%3d): ref_count(%5d), index(%5d), sockfd(%5d), host(%15s), port(%5d)!\n", __func__, __LINE__, ptr->ref_count, ptr->index, ptr->sockfd, ptr->host, ptr->port)
#else
	#define _conn_info_ex(fd,ptr,append)
#endif

#define conn_info(ptr) _conn_info_ex(stdout,ptr,"[ conn_info ] ")
#define conn_info_ex(ptr,append) _conn_info_ex(stderr,ptr,append)

typedef void (*server_func_t)(void);
typedef void (*thread_func_t)(worker_thread_t*);
typedef void (*conn_func_t)(conn_t*);
typedef bool (*conn_accept_func_t)(conn_t*);
typedef int (*conn_recv_func_t)(conn_t*,const char *, int, volatile char **);

typedef struct
{
	char *key;
	conn_recv_func_t call;
} conn_recv_t;

typedef struct
{
	server_func_t start,stop;
	thread_func_t thread_init,thread_destory;
	conn_accept_func_t conn_accept;
	conn_func_t conn_denied,conn_close;
	conn_recv_t *conn_recvs;
	unsigned int conn_recv_len;
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

extern GHashTable *ht_conn_recvs;

#endif
