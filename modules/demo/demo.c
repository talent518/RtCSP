#include <stdio.h>
#include <stdbool.h>
#include "mod_demo.h"
#include "mod_my.h"

bool demo_conn_accept(conn_t *ptr) {
	MYSQL_BIND binds[4];
	char query[]="INSERT IGNORE INTO session(sessId,sockfd,host,port,dateline,createTime)VALUES(?,?,?,?,UNIX_TIMESTAMP(),NOW())";

	memset(binds, 0, sizeof(binds));

	binds[0].buffer_type = MYSQL_TYPE_LONG;
	binds[0].buffer = &ptr->index;

	binds[1].buffer_type = MYSQL_TYPE_LONG;
	binds[1].buffer = &ptr->sockfd;

	binds[2].buffer_type = MYSQL_TYPE_STRING;
	binds[2].buffer = ptr->host;
	binds[2].buffer_length = strlen(ptr->host);

	binds[3].buffer_type = MYSQL_TYPE_LONG;
	binds[3].buffer = &ptr->port;

	MOD_MY_QUERY(stmt, query, strlen(query), binds) {
		return false;
	} MOD_MY_QUERY_CLOSE(stmt, query) {
		return false;
	} MOD_MY_QUERY_END();

	conn_info_ex(ptr,"accept connection run in main thread");

	return true;
}

void demo_conn_denied(conn_t *ptr) {
	conn_info_ex(ptr,"deined connection run in main thread");
}

void demo_conn_close(conn_t *ptr) {
	MYSQL_BIND bind;
	char query[] = "DELETE FROM session WHERE sessId=?";
	
	memset(&bind, 0, sizeof(bind));

	bind.buffer_type = MYSQL_TYPE_LONG;
	bind.buffer = &ptr->index;

	MOD_MY_QUERY_PTR_ONLY(stmt, query, strlen(query), &bind);
	
	conn_info_ex(ptr,"close connection");
}

void demo_thread_init(worker_thread_t *thread) {
	printf("%s[%d]...\n",__func__, thread->id);
}

void demo_thread_destory(worker_thread_t *thread) {
	printf("%s[%d]...\n",__func__, thread->id);
}

void demo_start() {
	char query[] = "DELETE FROM session";
	if (mysql_query(MOD_MY_CONN, query)) {
		fprintf(stderr, "%s\n", mysql_error(MOD_MY_CONN));
		return;
	}
	
	printf("%s...\n",__func__);
}

void demo_stop() {
	char query[] = "DELETE FROM session";
	if (mysql_query(MOD_MY_CONN, query)) {
		fprintf(stderr, "%s\n", mysql_error(MOD_MY_CONN));
		return;
	}

	printf("%s...\n",__func__);
}

bool demo_string(conn_t *ptr, const char *data, int datalen, GString *gstr) {
	g_string_append_printf(gstr, "%s...(%d: %s)", __func__, datalen, data);

	return true;
}

bool demo_mysql(conn_t *ptr, const char *data, int datalen, GString *gstr) {
	if (mysql_real_query(MOD_MY_CONN_PTR, data, datalen)) {
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
