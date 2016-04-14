#include <stdio.h>
#include <stdbool.h>
#include "mod_bench_demo.h"

bool demo_string_request(conn_t *ptr, GString *gstr) {
	g_string_append_printf(gstr, "%s...",__func__);

	return true;
}

bool demo_string_response(conn_t *ptr, const char *data, int datalen) {
	//printf("%s...(%d: %s)\n",__func__, datalen, data);

	return true;
}

bool demo_mysql_request(conn_t *ptr, GString *gstr) {
	g_string_append_printf(gstr, "SHOW DATABASES");

	return true;
}

bool demo_mysql_response(conn_t *ptr, const char *data, int datalen) {
	//printf("%s...(%d)\n",__func__, datalen);
	//printf("%s", data);

	return true;
}

void demo_start() {
	printf("%s...\n",__func__);
}

void demo_stop() {
	printf("%s...\n",__func__);
}

conn_send_recv_t demo_recvs[]={
	{"demo.string", sizeof("demo.string")-1, demo_string_request, demo_string_response},
	{"demo.mysql", sizeof("demo.mysql")-1, demo_mysql_request, demo_mysql_response}
};
bench_module_t demo_module={
	demo_start,
	demo_stop,
	demo_recvs,
	sizeof(demo_recvs)/sizeof(conn_send_recv_t)
};
