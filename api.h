#ifndef API_H
#define API_H

#include <stdio.h>

#ifdef DEBUG
	#define INIT_RUNTIME() double runtime=microtime(),tmpruntime
	#define INFO_RUNTIME(info) tmpruntime=microtime();printf("[ " info " ] %20s: %.3fs\n", __func__, tmpruntime-runtime);runtime=tmpruntime
	#define dprintf(...) printf(__VA_ARGS__)
#else
	#define INIT_RUNTIME()
	#define INFO_RUNTIME(info)
	#define dprintf(...)
#endif

#define tput_cols() execi("tput cols")
#define tput_lines() execi("tput lines")

double microtime();

char *fsize(int size);

char *gad(const char *argv0);

int execi(const char *cmd);

char *str_repeat(const char *str,size_t str_len,size_t repeat);
void strnprint(const char *str,size_t repeat);

#endif