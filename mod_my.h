#include "RtCSP.h"
#include <mysql.h>

extern rtcsp_module_t mod_my_module;

extern MYSQL *mod_my_conns;

extern char *mod_my_host;
extern char *mod_my_user;
extern char *mod_my_passwd;
extern char *mod_my_database;
extern unsigned int mod_my_port;
extern char *mod_my_socket;

#define MOD_MY_CONN MOD_MY_CONN_EX(0)
#define MOD_MY_CONN_PTR MOD_MY_CONN_EX(ptr->thread->id+1)
#define MOD_MY_CONN_EX(i) (&(mod_my_conns[i]))

#define MOD_MY_QUERY(stmt,sql,sqllen,binds) MOD_MY_QUERY_EX(0,stmt,sql,sqllen,binds)
#define MOD_MY_QUERY_ONLY(stmt,sql,sqllen,binds) MOD_MY_QUERY_EX(0,stmt,sql,sqllen,binds);MOD_MY_QUERY_CLOSE(stmt,sql);MOD_MY_QUERY_END()
#define MOD_MY_QUERY_PTR(stmt,sql,sqllen,binds) MOD_MY_QUERY_EX(ptr->thread->id+1,stmt,sql,sqllen,binds)
#define MOD_MY_QUERY_PTR_ONLY(stmt,sql,sqllen,binds) MOD_MY_QUERY_PTR(stmt,sql,sqllen,binds);MOD_MY_QUERY_CLOSE(stmt,sql);MOD_MY_QUERY_END()

// if query failure
#define MOD_MY_QUERY_EX(id,stmt,sql,sqllen,binds) \
	register MYSQL_STMT *stmt = mysql_stmt_init(MOD_MY_CONN_EX(id)); \
	if (mysql_stmt_prepare(stmt, sql, sqllen) || mysql_stmt_bind_param(stmt, binds) || mysql_stmt_execute(stmt)) { \
		fprintf(stderr, "Query error(prepare):\nError code: %u\nError message: %s\n", mysql_stmt_errno(stmt), mysql_stmt_error(stmt), sql); \
		mysql_stmt_close(stmt);

// stmt query success
#define MOD_MY_QUERY_SUCCESS(stmt) \
	} else {
	
// stmt query result process
#define MOD_MY_QUERY_RESULT(stmt) \
	} else { \
		mysql_stmt_store_result(stmt); \
		MYSQL_ROW row; \
		while ((row = mysql_fetch_row(res)) != NULL)

// if close stmt failure
#define MOD_MY_QUERY_CLOSE(stmt,sql) \
	} \
	if (mysql_stmt_close(stmt)) \
	{ \
		fprintf(stderr, "Query error(close):\nError code: %u\nError message: %s\n", mysql_stmt_errno(stmt), mysql_stmt_error(stmt), sql);

// end stmt
#define MOD_MY_QUERY_END() \
	}

