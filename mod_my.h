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
	ulong type_##stmt= CURSOR_TYPE_READ_ONLY; \
	if (mysql_stmt_prepare(stmt, sql, sqllen) || (binds && mysql_stmt_bind_param(stmt, binds)) || mysql_stmt_attr_set(stmt, STMT_ATTR_CURSOR_TYPE, (void*) &type_##stmt) || mysql_stmt_execute(stmt)) { \
		fprintf(stderr, "Query error(prepare):\nError code: %u\nError message: %s\n", mysql_stmt_errno(stmt), mysql_stmt_error(stmt), sql); \
		mysql_stmt_close(stmt);

// stmt query success
#define MOD_MY_QUERY_SUCCESS(stmt) \
	} else {

// begin process stmt query result
#define MOD_MY_QUERY_RESULT(stmt,sql) MOD_MY_QUERY_RESULT_EX(stmt,sql,res,fields,num_fields,bind_results,row,lengths,isnulls)
#define MOD_MY_QUERY_RESULT_EX(stmt,sql,res,fields,num_fields,bind_results,row,lengths,isnulls) \
	} else { \
		my_bool one_##stmt= 1; \
		if (mysql_stmt_attr_set(stmt, STMT_ATTR_UPDATE_MAX_LENGTH, (void*) &one_##stmt)) { \
			fprintf(stderr, "Query error(set max length attribute with field):\nError code: %u\nError message: %s\n", mysql_stmt_errno(stmt), mysql_stmt_error(stmt), sql); \
		} \
		if(mysql_stmt_field_count(stmt) && !mysql_stmt_store_result(stmt)) { \
			MYSQL_RES *res = mysql_stmt_result_metadata(stmt); \
			if(res) { \
				MYSQL_FIELD *fields = mysql_fetch_fields(res); \
				uint num_fields = mysql_num_fields(res); \
				MYSQL_BIND *bind_results = (MYSQL_BIND*)malloc(sizeof(MYSQL_BIND)*num_fields); \
				ulong *lengths = (ulong*)malloc(sizeof(ulong)*num_fields); \
				my_bool *isnulls = (my_bool*)malloc(sizeof(my_bool)*num_fields); \
				register uint i_##stmt; \
				register size_t max_length_##stmt; \
				char **row = (char**)malloc(sizeof(char*)*num_fields); \
				memset(bind_results, 0, sizeof(MYSQL_BIND)*num_fields); \
				memset(lengths, 0, sizeof(ulong)*num_fields); \
				memset(isnulls, 0, sizeof(my_bool)*num_fields); \
				for (i_##stmt = 0; i_##stmt < num_fields; i_##stmt++) { \
					max_length_##stmt = fields[i_##stmt].max_length + 1; \
					row[i_##stmt] = (char*)malloc(max_length_##stmt); \
					bind_results[i_##stmt].buffer_type = MYSQL_TYPE_STRING; \
					bind_results[i_##stmt].buffer = row[i_##stmt]; \
					bind_results[i_##stmt].buffer_length = (ulong)max_length_##stmt; \
					bind_results[i_##stmt].is_null = &isnulls[i_##stmt]; \
					bind_results[i_##stmt].length = &lengths[i_##stmt]; \
				} \
				if (mysql_stmt_bind_result(stmt, bind_results)) { \
					fprintf(stderr, "Query error(bind result):\nError code: %u\nError message: %s\n", mysql_stmt_errno(stmt), mysql_stmt_error(stmt), sql); \
				} else {

// end process stmt query result
#define MOD_MY_QUERY_RESULT_END(stmt,sql) MOD_MY_QUERY_RESULT_END_EX(stmt,sql,res,fields,num_fields,bind_results,row,lengths,isnulls)
#define MOD_MY_QUERY_RESULT_END_EX(stmt,sql,res,fields,num_fields,bind_results,row,lengths,isnulls) \
					if (mysql_stmt_fetch(stmt) != MYSQL_NO_DATA) { \
						fprintf(stderr, "Query error(still have data):\nError code: %u\nError message: %s\n", mysql_stmt_errno(stmt), mysql_stmt_error(stmt), sql); \
					} \
				} \
				for (i_##stmt=0; i_##stmt < num_fields; i_##stmt++) { \
					free(row[i_##stmt]); \
				} \
				free(bind_results); \
				free(row); \
				free(lengths); \
				free(isnulls); \
				mysql_free_result(res); \
			} else {\
				fprintf(stderr, "Query error(none result):\nError code: %u\nError message: %s\n", mysql_stmt_errno(stmt), mysql_stmt_error(stmt), sql); \
			} \
		} else {\
			fprintf(stderr, "Query error(cache result):\nError code: %u\nError message: %s\n", mysql_stmt_errno(stmt), mysql_stmt_error(stmt), sql); \
		} \
	MOD_MY_QUERY_CLOSE(stmt,sql)

// if close stmt failure
#define MOD_MY_QUERY_CLOSE(stmt,sql) \
	} \
	if (mysql_stmt_close(stmt)) \
	{ \
		fprintf(stderr, "Query error(close):\nError code: %u\nError message: %s\n", mysql_stmt_errno(stmt), mysql_stmt_error(stmt), sql);

// end stmt
#define MOD_MY_QUERY_END() \
	}

