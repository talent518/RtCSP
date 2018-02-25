#ifndef API_H
#define API_H

#include <stdio.h>

#ifdef DEBUG_RUN_TIME
	#define INIT_RUNTIME() double runtime=microtime(),tmpruntime
	#define INFO_RUNTIME(info) tmpruntime=microtime();printf("[ " info " ] %20s: %.3fs\n", __func__, tmpruntime-runtime);runtime=tmpruntime
#else
	#define INIT_RUNTIME()
	#define INFO_RUNTIME(info)
#endif

#define tput_cols() execi("tput cols")
#define tput_lines() execi("tput lines")

double microtime();

char *fsize(int size);

int execi(const char *cmd);

char *str_repeat(const char *str,size_t str_len,size_t repeat);
void strnprint(const char *str,size_t repeat);

#endif
