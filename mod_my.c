#include <stdio.h>
#include <stdbool.h>
#include <mysql_version.h>
#include "mod_my.h"
#include "loop_event.h"

char *mod_my_host = "localhost";
char *mod_my_user = "root";
char *mod_my_passwd = "1qazXSW23edc";
char *mod_my_database = "mysql";//"rtcsp";
unsigned int mod_my_port = 3306;
char *mod_my_socket = "/opt/lampp/var/mysql/mysql.sock";//MYSQL_UNIX_ADDR;
unsigned long mod_my_flags = CLIENT_COMPRESS | CLIENT_FOUND_ROWS | CLIENT_LOCAL_FILES;

MYSQL **mod_my_conns;
struct event *mod_my_events = NULL;
static struct timeval tv;

inline void mod_my_connect(int i) {
	if (!mysql_real_connect(mod_my_conns[i], mod_my_host, mod_my_user, mod_my_passwd, mod_my_database, mod_my_port, mod_my_socket, mod_my_flags)) {
		fprintf(stderr, "%s\n", mysql_error(mod_my_conns[i]));
		return 1;
	}

	char reconnect = 1;
	mysql_options(mod_my_conns[i], MYSQL_OPT_RECONNECT, &reconnect);
	
#if	 MYSQL_VERSION_ID > 40100
	mysql_query(mod_my_conns[i], "SET NAMES UTF-8");
	#if MYSQL_VERSION_ID > 50001
		mysql_query(mod_my_conns[i], "SET sql_mode=''");
	#endif
#endif
}

static void mod_my_timeout_handler(const int fd, short event, void *arg) {
	if(mysql_ping(mod_my_conns[(int)arg])) {
		mod_my_connect((int)arg);
	}
}

void mod_my_thread_init(worker_thread_t *thread) {
	mod_my_conns[thread->id+1] = mysql_init(NULL);

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
	mysql_close(mod_my_conns[thread->id+1]);
	
	event_del(&mod_my_events[thread->id+1]);
	
	printf("%s[%d]...\n",__func__, thread->id);
}

void mod_my_start() {
	printf("%s...\n",__func__);
	mod_my_conns = (MYSQL**)malloc(sizeof(MYSQL*)*(rtcsp_nthreads+1));

	mod_my_conns[0] = mysql_init(NULL);

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
	mysql_close(mod_my_conns[0]);
	
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
