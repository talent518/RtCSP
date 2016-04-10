#include <stdio.h>
#include <stdbool.h>
#include "mod_demo.h"
#include "mod_my.h"

bool demo_conn_accept(conn_t *ptr) {
	MYSQL_STMT   *stmt;
	MYSQL_BIND   binds[4];
	
	stmt = mysql_stmt_init(MOD_MY_CONN);
	if (!stmt)
	{
		fprintf(stderr, "mysql_stmt_init()\n");
		return false;
	}
	
	char query[]="INSERT IGNORE INTO session(sessId,sockfd,host,port,dateline,createTime)VALUES(?,?,?,?,UNIX_TIMESTAMP(),NOW())";
	
	if (mysql_stmt_prepare(stmt, query, strlen(query)))
	{
		fprintf(stderr, "mysql_stmt_prepare(stmt, query, strlen(query)): %s: %s\n", mysql_stmt_error(stmt), query);
		return false;
	}
	
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

	if (mysql_stmt_bind_param(stmt, binds))
	{
		fprintf(stderr, "mysql_stmt_bind_param(stmt, binds): %s: %s\n", mysql_stmt_error(stmt), query);
		return false;
	}

	if (mysql_stmt_execute(stmt))
	{
		fprintf(stderr, "mysql_stmt_execute(stmt): %s: %s\n", mysql_stmt_error(stmt), query);
		return false;
	}
	
	if (mysql_stmt_close(stmt))
	{
		fprintf(stderr, "mysql_stmt_close(stmt): %s: %s\n", mysql_stmt_error(stmt), query);
		return false;
	}

	conn_info_ex(ptr,"accept connection run in main thread");

	return true;
}

void demo_conn_denied(conn_t *ptr) {
	conn_info_ex(ptr,"deined connection run in main thread");
}

void demo_conn_close(conn_t *ptr) {
	MYSQL_STMT   *stmt;
	MYSQL_BIND   bind;
	
	stmt = mysql_stmt_init(MOD_MY_CONN_PTR);
	if (!stmt)
	{
		fprintf(stderr, "mysql_stmt_init()\n");
		return;
	}
	
	char query[] = "DELETE FROM session WHERE sessId=?";
	
	if (mysql_stmt_prepare(stmt, query, strlen(query)))
	{
		fprintf(stderr, "mysql_stmt_prepare(stmt, query, strlen(query)): %s: %s\n", mysql_stmt_error(stmt), query);
		return;
	}
	
	memset(&bind, 0, sizeof(bind));

	bind.buffer_type = MYSQL_TYPE_LONG;
	bind.buffer = &ptr->index;

	if (mysql_stmt_bind_param(stmt, &bind))
	{
		fprintf(stderr, "mysql_stmt_bind_param(stmt, binds): %s: %s\n", mysql_stmt_error(stmt), query);
		return;
	}

	if (mysql_stmt_execute(stmt))
	{
		fprintf(stderr, "mysql_stmt_execute(stmt): %s: %s\n", mysql_stmt_error(stmt), query);
		return;
	}
	
	if (mysql_stmt_close(stmt))
	{
		fprintf(stderr, "mysql_stmt_close(stmt): %s: %s\n", mysql_stmt_error(stmt), query);
		return;
	}

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
