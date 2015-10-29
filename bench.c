#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "config.h"
#include "bench.h"
#include "api.h"
#include "getopt.h"

char *bench_host = "127.0.0.1";
unsigned int bench_port = 8083;
unsigned int bench_nthreads;
unsigned int bench_prethread_conns = 10;
unsigned int bench_preconn_requests = 10;

int bench_maxrecvs = 2*1024*1024;

#define OPT_HOST 1
#define OPT_PORT 2
#define OPT_NTHREADS 3
#define OPT_CONNS 4
#define OPT_REQUESTS 5
#define OPT_MAX_RECVS 6

volatile char *bench_optarg = NULL;
volatile int bench_optind = 1;

static const opt_struct OPTIONS[] =
{
	{'h', 0, "help"},
	{'?', 0, "usage"},/* help alias (both '?' and 'usage') */
	{'H', 0, "hide-args"},
	{'v', 0, "version"},
	{'m', 0, "module"},

	{OPT_HOST,  1, "host"},
	{OPT_PORT,  1, "port"},

	{OPT_NTHREADS,  1, "nthreads"},
	{OPT_CONNS,  1, "conns"},
	{OPT_REQUESTS,  1, "requests"},

	{OPT_MAX_RECVS,  1, "max-recvs"},

	{'-', 0, NULL} /* end of args */
};

/* {{{ bench_usage
 */
static void bench_usage(char *argv0)
{
	char *prog;

	prog = strrchr(argv0, '/');
	if (prog)
	{
		prog++;
	}
	else
	{
		prog = RTCSP_NAME;
	}

	char *maxrecvs;

	maxrecvs=(char*)fsize(bench_maxrecvs);

	printf( "Usage: %s [options] [args]\n"
			"\n"
			"  options:\n"
			"  -h,-?                   This help\n"
			"  -H                      Hide any passed arguments from external tools\n"
			"  -v                      Version number\n"
			"  -m                      The display module list\n"
			"\n"
			"  --host <IP>             Host IP (default: %s)\n"
			"  --port <port>           Host port (default: %d)\n"
			"  --nthreads <number>     Concurrent thread number (default: %d)\n"
			"  --conns <number>        Number of connections per thread (default: %d)\n"
			"  --requests <number>     Number of requests per connection (default: %d)\n"
			"  --max-recvs <size>      Max recv data size (default: %s)\n"
			"\n"
			, prog, bench_host, bench_port, bench_nthreads, bench_prethread_conns, bench_preconn_requests, maxrecvs);

	free(maxrecvs);
}
/* }}} */

int main(int argc, char *argv[]) {
#ifdef HAVE_SIGNAL_H
#if defined(SIGPIPE) && defined(SIG_IGN)
	signal(SIGPIPE, SIG_IGN);
#endif
#if defined(SIGCHLD) && defined(SIG_IGN)
	signal(SIGCHLD,SIG_IGN);
#endif
#endif

	int exit_status = 0;
	int c,i,j;
	/* temporary locals */
	int hide_argv = 0;
	/* end of temporary locals */

	bench_nthreads = sysconf(_SC_NPROCESSORS_CONF)*2;

	if (argc==1)
	{
		goto begin;
	}

	while ((c = rtcsp_getopt(argc, argv, OPTIONS, &bench_optarg, &bench_optind, 1, 2))!=-1)
	{
		switch (c)
		{
			case OPT_HOST:
				bench_host=strdup(bench_optarg);
				break;
			case OPT_PORT:
				bench_port=atoi(bench_optarg);
				break;
			case OPT_NTHREADS:
				bench_nthreads=atoi(bench_optarg);
				break;
			case OPT_CONNS:
				bench_prethread_conns=atoi(bench_optarg);
				break;
			case OPT_REQUESTS:
				bench_preconn_requests=atoi(bench_optarg);
				break;
			case OPT_MAX_RECVS:
				bench_maxrecvs=atoi(bench_optarg);
				break;
			case 'h': /* help & quit */
			case '?':
				bench_usage(argv[0]);
				exit_status=0;
				goto out;
			case 'v': /* show RtCSPb version & quit */
				printf("rtcspb %s (built: %s %s)\nCopyright (c) 1997-2012 The Abao\n", RTCSP_VERSION, __DATE__, __TIME__);
				exit_status=0;
				goto out;
				break;

			case 'm': /* module list */
				for(i=0;i<bench_length;i++) {
					printf("%s: ",bench_names[i]);
					for(j=0;j<bench_modules[i]->recvs_len;j++) {
						printf("%s ", bench_modules[i]->recvs[j].key);
					}
					printf("\n");
				}
				goto out;
				break;

			case 'H':
				hide_argv = 1;
				break;
			default:
				break;
		}
	}

	if (hide_argv)
	{
		int i;
		for (i = 1; i < argc; i++)
		{
			memset(argv[i], 0, strlen(argv[i]));
		}
	}

begin:
	loop_event();

	return 0;

out:
err:
	exit(exit_status);
	return 0;
}