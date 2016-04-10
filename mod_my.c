#include <stdio.h>
#include <stdbool.h>
#include <mysql_version.h>
#include <glib.h>
#include "mod_my.h"
#include "loop_event.h"

char *mod_my_host = "localhost";
char *mod_my_user = "root";
char *mod_my_passwd = "1qazXSW23edc";
char *mod_my_database = "rtcsp";
unsigned int mod_my_port = 3306;
char *mod_my_socket = MYSQL_UNIX_ADDR;
unsigned long mod_my_flags = CLIENT_COMPRESS | CLIENT_FOUND_ROWS | CLIENT_LOCAL_FILES;

MYSQL *mod_my_conns;
struct event *mod_my_events = NULL;
static struct timeval tv;

#define g_print_error(msg,cond,err) \
	if(err) { \
		if(cond) { \
			fprintf(stderr, msg"code: %d\nmessage: %s\n", err->code, err->message); \
		} \
		g_error_free(err); \
		err = NULL; \
	}
#define g_print_error_not(msg,noncode,err) g_print_error(msg,err->code!=noncode,err)
#define g_print_error_eq(msg,eqcode,err) g_print_error(msg,err->code==eqcode,err)

inline void mod_my_connect(int i) {
	char reconnect = 1;
	mysql_options(MOD_MY_CONN_EX(i), MYSQL_OPT_RECONNECT, &reconnect);
	
	if (!mysql_real_connect(MOD_MY_CONN_EX(i), mod_my_host, mod_my_user, mod_my_passwd, mod_my_database, mod_my_port, mod_my_socket, mod_my_flags)) {
		fprintf(stderr, "mysql connect error: %s\n", mysql_error(MOD_MY_CONN_EX(i)));
		return;
	}
	
#if	 MYSQL_VERSION_ID > 40100
	mysql_query(MOD_MY_CONN_EX(i), "SET NAMES UTF-8");
	#if MYSQL_VERSION_ID > 50001
		mysql_query(MOD_MY_CONN_EX(i), "SET sql_mode=''");
	#endif
#endif
}

static void mod_my_timeout_handler(const int fd, short event, void *arg) {
	int i = (int)arg;
	if(mysql_ping(MOD_MY_CONN_EX(i))) {
		mysql_close(MOD_MY_CONN_EX(i));
		mod_my_connect(i);
	}
}

void mod_my_thread_init(worker_thread_t *thread) {
	mysql_init(MOD_MY_CONN_EX(thread->id+1));

	mysql_thread_init();

	mod_my_connect(thread->id+1);
	
	// timeout event
	event_set(&mod_my_events[thread->id+1], -1, EV_PERSIST, mod_my_timeout_handler, (void*)(thread->id+1));
	event_base_set(thread->base, &mod_my_events[thread->id+1]);
	if (event_add(&mod_my_events[thread->id+1], &tv) == -1) {
		perror("timeout event");
		exit(1);
	}
	
	printf("%s[%d]...\n",__func__, thread->id);
}

void mod_my_thread_destory(worker_thread_t *thread) {
	mysql_close(MOD_MY_CONN_EX(thread->id+1));
	mysql_thread_end();
	
	event_del(&mod_my_events[thread->id+1]);
	
	printf("%s[%d]...\n",__func__, thread->id);
}

void mod_my_start() {
	GKeyFile *keyfile = g_key_file_new();
	GError *error = NULL;
	
	if(g_key_file_load_from_file(keyfile, SYS_CONF_DIR"/rtcsp.ini", G_KEY_FILE_NONE, &error)) {
		char *host = g_key_file_get_string(keyfile, "my", "host", &error);
		if(host) {
			mod_my_host = host;
			g_hash_table_insert(ht_main_free, host, free);
		}
		g_print_error_eq("config argument \"host\" for \"my\" error:\n", G_KEY_FILE_ERROR_INVALID_VALUE, error);
		
		char *user = g_key_file_get_string(keyfile, "my", "user", &error);
		if(user) {
			mod_my_user = user;
			g_hash_table_insert(ht_main_free, user, free);
		}
		g_print_error_eq("config argument \"user\" for \"my\" error:\n", G_KEY_FILE_ERROR_INVALID_VALUE, error);
		
		char *passwd = g_key_file_get_string(keyfile, "my", "passwd", &error);
		if(passwd) {
			mod_my_passwd = passwd;
			g_hash_table_insert(ht_main_free, passwd, free);
		}
		g_print_error_eq("config argument \"passwd\" for \"my\" error:\n", G_KEY_FILE_ERROR_INVALID_VALUE, error);
		
		char *database = g_key_file_get_string(keyfile, "my", "database", &error);
		if(database) {
			mod_my_database = database;
			g_hash_table_insert(ht_main_free, database, free);
		}
		g_print_error_eq("config argument \"database\" for \"my\" error:\n", G_KEY_FILE_ERROR_INVALID_VALUE, error);
		
		int port = g_key_file_get_integer(keyfile, "my", "port", &error);
		if(port > 0) {
			mod_my_port = port;
		}
		g_print_error_eq("config argument \"port\" for \"my\" error:\n", G_KEY_FILE_ERROR_INVALID_VALUE, error);
		
		char *socket = g_key_file_get_string(keyfile, "my", "socket", &error);
		if(socket) {
			mod_my_socket = socket;
			g_hash_table_insert(ht_main_free, socket, free);
		}
		g_print_error_eq("config argument \"socket\" for \"my\" error:\n", G_KEY_FILE_ERROR_INVALID_VALUE, error);
	}
	g_print_error_not("config file \""SYS_CONF_DIR"/rtcsp.ini\" not exists or have error:\n", G_FILE_ERROR_NOENT, error);
	g_key_file_free(keyfile);

	mod_my_conns = (MYSQL*)malloc(sizeof(MYSQL)*(rtcsp_nthreads+1));
	
	memset(mod_my_conns, 0, sizeof(MYSQL)*(rtcsp_nthreads+1));
	
	mysql_library_init(0, NULL, NULL);

	mysql_init(MOD_MY_CONN);
	mysql_thread_init();

	mod_my_connect(0);

	mod_my_events = (struct event *)malloc(sizeof(struct event)*(rtcsp_nthreads+1));
	
	// timeout event
	evutil_timerclear(&tv);
	tv.tv_sec = 10;
	event_set(&mod_my_events[0], -1, EV_PERSIST, mod_my_timeout_handler, NULL);
	event_base_set(listen_thread.base, &mod_my_events[0]);
	if (event_add(&mod_my_events[0], &tv) == -1) {
		perror("timeout event");
		exit(1);
	}

	printf("%s...\n",__func__);
}

void mod_my_stop() {
	mysql_close(MOD_MY_CONN);
	mysql_thread_end();
	mysql_library_end();
	
	free(mod_my_conns);
	
	event_del(&mod_my_events[0]);
	free(mod_my_events);
	
	printf("%s...\n",__func__);
}

rtcsp_module_t mod_my_module={
	mod_my_start,
	mod_my_stop,
	mod_my_thread_init,
	mod_my_thread_destory,
	NULL,
	NULL,
	NULL,
	NULL,
	0
};
