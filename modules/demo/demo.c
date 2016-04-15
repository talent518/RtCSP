#include <stdio.h>
#include <stdbool.h>
#include "mod_demo.h"
#include "mod_my.h"

bool demo_conn_accept(conn_t *ptr) {
	MYSQL_BIND binds[5];
	char query[]="INSERT IGNORE INTO session(sessId,sockfd,host,port,tid,dateline,createTime)VALUES(?,?,?,?,?,UNIX_TIMESTAMP(),NOW())";

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

	binds[4].buffer_type = MYSQL_TYPE_LONG;
	binds[4].buffer = &ptr->thread->id;

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
	MYSQL_BIND bind;
	char query[] = "SELECT * FROM session WHERE tid=?";
	
	memset(&bind, 0, sizeof(bind));

	bind.buffer_type = MYSQL_TYPE_LONG;
	bind.buffer = &ptr->thread->id;

	MOD_MY_QUERY_PTR(stmt, query, strlen(query), &bind) {
		return false;
	} MOD_MY_QUERY_RESULT(stmt, query) {
		char formats[num_fields][20];
		ulong row_length = 0;
		for(i_stmt=0; i_stmt<num_fields; i_stmt++) {
			max_length_stmt = (fields[i_stmt].type == MYSQL_TYPE_TIMESTAMP || fields[i_stmt].type == MYSQL_TYPE_DATETIME ? 20 : fields[i_stmt].max_length+1);
			row_length += max_length_stmt;
			sprintf(formats[i_stmt], "%%%us", max_length_stmt);
		}
		char line[row_length+2];
		memset(line, '=', sizeof(line));
		line[row_length] = '\n';
		line[row_length+1] = '\0';
		g_string_append(gstr, line);
		for(i_stmt=0; i_stmt<num_fields; i_stmt++) {
			g_string_append_printf(gstr, formats[i_stmt], fields[i_stmt].name);
		}
		g_string_append_c(gstr, '\n');
		g_string_append(gstr, line);
		memset(line, '-', sizeof(line)-2);
		unsigned int n = mysql_stmt_num_rows(stmt);
		if(n) {
			while (!mysql_stmt_fetch(stmt)) {
				for(i_stmt=0; i_stmt<num_fields; i_stmt++) {
					g_string_append_printf(gstr, formats[i_stmt], row[i_stmt]);
				}
				g_string_append_c(gstr, '\n');
				g_string_append(gstr, line);
			}
		}
	} MOD_MY_QUERY_RESULT_END(stmt, query) {
		return false;
	} MOD_MY_QUERY_END();

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
