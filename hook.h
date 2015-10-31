#ifdef _HOOK_H
#define _HOOK_H

#include <stdbool.h>

#include "conn.h"
#include "loop_event.h"

inline bool hook_conn_accept(conn_t *ptr);
inline void hook_conn_close(conn_t *ptr);
void hook_conn_recv(conn_t *ptr,const char *data, int data_len);
inline void hook_conn_denied(conn_t *ptr);

inline void hook_thread_init(worker_thread_t *thread);
inline void hook_thread_destory(worker_thread_t *thread);
inline void hook_start();
inline void hook_stop();

#endif