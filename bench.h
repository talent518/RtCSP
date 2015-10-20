#ifndef _BENCH_H
#define _BENCH_H

#include <stdlib.h>
#include <event.h>

#define rtcsp_maxrecvs bench_maxrecvs

#ifdef DEBUG
#define dprintf(...) fprintf(stdout,__VA_ARGS__)
#define conn_info(ptr) _conn_info_ex(stdout,ptr,"[ conn_info ] ")
#else
#define dprintf(...)
#define conn_info(ptr)
#endif

#define _conn_info(ptr) _conn_info_ex(stdout,ptr,"[ _conn_info ] ")

#define _conn_info_ex(fd,ptr,append) fprintf(fd,append" in %20s on line (%3d): index(%5d), sockfd(%5d), host(%15s), port(%5d)!\n", __func__, __LINE__, ptr->index, ptr->sockfd, ptr->host, ptr->port)
#define conn_info_ex(ptr,append) _conn_info_ex(stderr,ptr,append)

typedef struct _conn_t{
	int index;
	
	int sockfd;
	char host[15];
	int port;

	char *rbuf;
	int rbytes;
	int rsize;

	struct event event;
} conn_t;

typedef int (*conn_recv_func_t)(conn_t *, const char *, int, char **);

typedef struct
{
	char *key;
	conn_recv_func_t call;
} conn_recv_t;

typedef struct
{
	conn_recv_t *recvs;
	size_t recvs_len;
} bench_module_t;

extern char bench_host[100];
extern unsigned int bench_port;
extern unsigned int bench_nthreads;
extern unsigned int bench_requests;
extern int bench_maxrecvs;

extern int bench_length;
extern char *bench_names[];
extern bench_module_t *bench_modules[];

int recv_data_len(conn_t *ptr);
int socket_recv(conn_t *ptr,char **data,int *data_len);
int socket_send(conn_t *ptr,const char *data,int data_len);

void socket_close(conn_t *ptr);

#endif