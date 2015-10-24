#include <stdio.h>
#include <stdbool.h>
#include "mod_bench_demo.h"

bool demo_user_request(conn_t *ptr, GString *gstr) {
	g_string_append_printf(gstr, "%s...",__func__);

	return true;
}

bool demo_user_response(conn_t *ptr, const char *data, int datalen) {
	//printf("%s...(%d: %s)\n",__func__, datalen, data);

	return true;
}

bool demo_profile_request(conn_t *ptr, GString *gstr) {
	g_string_append_printf(gstr, "%s...",__func__);

	return true;
}

bool demo_profile_response(conn_t *ptr, const char *data, int datalen) {
	//printf("%s...(%d: %s)\n",__func__, datalen, data);

	return true;
}

conn_send_recv_t demo_recvs[]={
	{"demo.user", sizeof("demo.user")-1, demo_user_request, demo_user_response},
	{"demo.profile", sizeof("demo.profile")-1, demo_profile_request, demo_profile_response}
};
bench_module_t demo_module={
	demo_recvs,
	sizeof(demo_recvs)/sizeof(conn_send_recv_t)
};