#include <stdio.h>
#include <stdbool.h>
#include "mod_demo.h"
#include "mod_my.h"

bool demo_conn_accept(conn_t *ptr) {
	conn_info_ex(ptr,"accept connection");

	return true;
}

void demo_conn_close(conn_t *ptr) {
	conn_info_ex(ptr,"close connection");
}

void demo_conn_denied(conn_t *ptr) {
	conn_info_ex(ptr,"deined connection");
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

bool demo_string(conn_t *ptr, const char *data, int datalen, GString *gstr) {
	g_string_append_printf(gstr, "%s...(%d: %s)", __func__, datalen, data);

	return true;
}

bool demo_mysql(conn_t *ptr, const char *data, int datalen, GString *gstr) {
	if (mysql_query(MOD_MY_CONN_PTR, data)) {
		fprintf(stderr, "%s\n", mysql_error(MOD_MY_CONN_PTR));
		return false;
	}
	
	MYSQL_RES *res;
	MYSQL_ROW row;
	
	res = mysql_use_result(MOD_MY_CONN_PTR);

	while ((row = mysql_fetch_row(res)) != NULL) {
   		g_string_append(gstr, row[0]);
   		g_string_append_c(gstr, '\n');
   	}

	mysql_free_result(res);

	return true;
}

conn_recv_t demo_recvs[]={
	{"demo.string", demo_string},
	{"demo.mysql", demo_mysql}
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
