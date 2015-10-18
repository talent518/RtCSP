#include "mod_demo.h"

int demo_conn_accept(conn_t *ptr) {
	conn_info_ex(ptr,"accept connection");

	return 0;
}

void demo_conn_close(conn_t *ptr) {
	conn_info_ex(ptr,"close connection");
}

void demo_conn_denied(conn_t *ptr) {
}

void demo_thread_init(worker_thread_t *thread) {
	printf("%s[%d]...\n",__func__, thread->id);
}

void demo_thread_destory(worker_thread_t *thread) {
	printf("%s[%d]...\n",__func__, thread->id);
}

void demo_start() {
	printf("%s...\n",__func__);
}

void demo_stop() {
	printf("%s...\n",__func__);
}

int demo_user(conn_t *ptr, const char *data, int datalen, char **retbuf) {
	conn_info(ptr);

	return 0;
}

int demo_profile(conn_t *ptr, const char *data, int datalen, char **retbuf) {
	conn_info(ptr);

	return 0;
}

conn_recv_t demo_recvs[]={
	{"demo.user", demo_user},
	{"demo.profile", demo_profile}
};
rtcsp_module_t demo_module={
	demo_start,
	demo_stop,
	demo_thread_init,
	demo_thread_destory,
	demo_conn_accept,
	demo_conn_denied,
	demo_conn_close,
	demo_recvs,
	sizeof(demo_recvs)/sizeof(conn_recv_t)
};