#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench.h"

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