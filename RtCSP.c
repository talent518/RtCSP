#include "config.h"
#include "getopt.h"
#include "RtCSP.h"

#include <stdio.h>
#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <signal.h>
#if HAVE_SETLOCALE
#include <locale.h>
#endif

#define OPT_HOST 1
#define OPT_PORT 2
#define OPT_PIDFILE 3
#define OPT_USER 4
#define OPT_MAX_CLIENTS 5
#define OPT_MAX_RECVS 6
#define OPT_NTHREADS 7

volatile char *rtcsp_optarg = NULL;
volatile int rtcsp_optind = 1;

static const opt_struct OPTIONS[] =
{
	{'h', 0, "help"},
	{'?', 0, "usage"},/* help alias (both '?' and 'usage') */
	{'H', 0, "hide-args"},
	{'v', 0, "version"},
	{'m', 0, "module"},
	{'b', 1, "backlog"},
	{'s', 1, "service"},

	{OPT_HOST,  1, "host"},
	{OPT_PORT,  1, "port"},
	{OPT_PIDFILE,  1, "pidfile"},

	{OPT_USER,  1, "user"},
	{OPT_NTHREADS,  1, "nthreads"},
	{OPT_MAX_CLIENTS,  1, "max-clients"},
	{OPT_MAX_RECVS,  1, "max-recvs"},

	{'-', 0, NULL} /* end of args */
};

/* {{{ rtcsp_usage
 */
static void rtcsp_usage(char *argv0)
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

	maxrecvs=(char*)fsize(rtcsp_maxrecvs);

	printf( "Usage: %s [options] [args]\n"
			"\n"
			"  options:\n"
			"  -h,-?                   This help\n"
			"  -H                      Hide any passed arguments from external tools\n"
			"  -v                      Version number\n"
			"  -m                      The display module list\n"
			"\n"
			"  --host <IP>             Listen host (default: %s)\n"
			"  --port <port>           Listen port (default: %d)\n"
			"  --pidfile <file>        Service pidfile (default: %s)\n"
			"  --user <username>       Run for user (default: %s)\n"
			"  --nthreads <number>     LibEvent thread number (default: %d)\n"
			"  --max-clients <number>  Max client connect number (default: %d)\n"
			"  --max-recvs <size>      Max recv data size (default: %s)\n"
			"  -b <backlog>            Set the backlog queue limit (default: %d)\n"
			"\n"
			"  -s <option>             service option\n"
			"  option:\n"
			"       start              start service\n"
			"       stop               stop service\n"
			"       restart            restart service\n"
			"       status             service status\n"
			"\n"
			, prog, rtcsp_host, rtcsp_port, rtcsp_pidfile, rtcsp_user, rtcsp_nthreads, rtcsp_maxclients, maxrecvs, rtcsp_backlog);

	free(maxrecvs);
}
/* }}} */

int server_start();
int server_stop();
int server_status();

int main(int argc, char *argv[])
{
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
	char *serv_opt=NULL;
	int hide_argv = 0;
	/* end of temporary locals */

	if (argc==1)
	{
		argc=2;
		argv[1]="-?";
	}

	while ((c = rtcsp_getopt(argc, argv, OPTIONS, &rtcsp_optarg, &rtcsp_optind, 0, 2))!=-1)
	{
		switch (c)
		{
			case OPT_HOST:
				rtcsp_host=strdup(rtcsp_optarg);
				break;
			case OPT_PORT:
				rtcsp_port=atoi(rtcsp_optarg);
				break;
			case OPT_PIDFILE:
				rtcsp_pidfile=strdup(rtcsp_optarg);
				break;

			case OPT_USER:
				rtcsp_user = strdup(rtcsp_optarg);
				break;
			case OPT_MAX_CLIENTS:
				rtcsp_maxclients=atoi(rtcsp_optarg);
				break;
			case OPT_MAX_RECVS:
				rtcsp_maxrecvs=atoi(rtcsp_optarg);
				break;
			case OPT_NTHREADS:
				rtcsp_nthreads=atoi(rtcsp_optarg);
				break;
			case 'b':
				rtcsp_backlog=atoi(rtcsp_optarg);
				break;
			case 'h': /* help & quit */
			case '?':
				rtcsp_usage(argv[0]);
				exit_status=0;
				goto out;
			case 'v': /* show RtCSP version & quit */
				printf("RtCSP %s (built: %s %s) %s\nCopyright (c) 1997-2012 The Abao\n%s", RTCSP_NAME, __DATE__, __TIME__);
				exit_status=0;
				goto out;
				break;

			case 'm': /* module list */
				for(i=0;i<rtcsp_length;i++) {
					printf("%s: ",rtcsp_names[i]);
					for(j=0;j<rtcsp_modules[i]->conn_recv_len;j++) {
						printf("%s ", rtcsp_modules[i]->conn_recvs[j].key);
					}
					printf("\n");
				}
				goto out;
				break;

			case 's': /* service control */
				serv_opt = strdup(rtcsp_optarg);
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

	if (serv_opt==NULL)
	{
		exit_status=0;
		goto err;
	}
	else if (strcmp(serv_opt,"restart")==0)
	{
		server_stop();
		server_start();
	}
	else if (strcmp(serv_opt,"stop")==0)
	{
		server_stop();
	}
	else if (strcmp(serv_opt,"start")==0)
	{
		server_start();
	}
	else if (strcmp(serv_opt,"status")==0)
	{
		server_stop();
	}
	else if (serv_opt)
	{
		rtcsp_usage(argv[0]);
		exit_status=1;
		goto out;
	}
	free(serv_opt);

	return 0;

out:
err:
	exit(exit_status);
}
