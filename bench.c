#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench.h"

char bench_host[100] = "127.0.0.1";
unsigned int bench_port = 8083;
unsigned int bench_nthreads = 10;
unsigned int bench_requests = 20;
int bench_maxrecvs = 2*1024*1024;

int main(int argc, char *argv[]) {
	int i,j;

	for(i=0;i<bench_length;i++) {
		printf("%s: ",bench_names[i]);
		for(j=0;j<bench_modules[i]->recvs_len;j++) {
			printf("%s ", bench_modules[i]->recvs[j].key);
		}
		printf("\n");
	}

	return 0;
}