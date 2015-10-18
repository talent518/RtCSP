#ifndef _BENCH_H
#define _BENCH_H

#include <stdlib.h>

typedef struct _conn_t{
	int index;
	
	int sockfd;
	char host[15];
	int port;

	char *rbuf;
	int rbytes;
	int rsize;
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

extern int bench_length;
extern char *bench_names[];
extern bench_module_t *bench_modules[];

#endif