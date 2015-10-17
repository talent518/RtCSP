#include "RtCSP.h"
#include "conn.h"
#include "hook.h"

int hook_conn_accept(conn_t *ptr) {
	conn_info_ex(ptr,"accept connection");

	return 1;
}

void hook_conn_close(conn_t *ptr) {
	conn_info_ex(ptr,"close connection");
}

void hook_conn_recv(conn_t *ptr,const char *data, int data_len) {
}

void hook_conn_denied(conn_t *ptr) {
}

void hook_thread_init(worker_thread_t *thread) {
	printf("%s[%d]...\n",__func__, thread->id);
}

void hook_thread_destory(worker_thread_t *thread) {
	printf("%s[%d]...\n",__func__, thread->id);
}

void hook_start() {
	int i;
	for(i=0;i<rtcsp_length;i++)
		printf("%s[%s]...\n",__func__,rtcsp_names[i]);
}

void hook_stop() {
	int i;
	for(i=0;i<rtcsp_length;i++)
		printf("%s[%s]...\n",__func__,rtcsp_names[i]);
}
