#include "mod_bench.h"

int demo_user(conn_t *ptr, const char *data, int datalen, char **retbuf) {
	printf("%s...\n",__func__);

	return 0;
}

int demo_profile(conn_t *ptr, const char *data, int datalen, char **retbuf) {
	printf("%s...\n",__func__);

	return 0;
}

conn_recv_t demo_recvs[]={
	{"demo.user", demo_user},
	{"demo.profile", demo_profile}
};
bench_module_t demo_module={
	demo_recvs,
	sizeof(demo_recvs)/sizeof(conn_recv_t)
};