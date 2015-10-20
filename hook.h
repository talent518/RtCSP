#ifdef _HOOK_H
#define _HOOK_H

#include "conn.h"
#include "loop_event.h"

int hook_conn_accept(conn_t *ptr);
void hook_conn_close(conn_t *ptr);
void hook_conn_recv(conn_t *ptr,const char *data, int data_len);
void hook_conn_denied(conn_t *ptr);

void hook_thread_init(worker_thread_t *thread);
void hook_thread_destory(worker_thread_t *thread);
void hook_start();
void hook_stop();

#endif