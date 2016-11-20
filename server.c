#include <sys/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <sys/types.h>
#include <signal.h>
#include <assert.h>

#include "config.h"
#include "RtCSP.h"
#include "conn.h"
#include "api.h"
#include "loop_event.h"
#include "socket.h"

#define flush() fflush(stdout)

unsigned int rtcsp_backlog=1024;

char *rtcsp_host="0.0.0.0";
short int rtcsp_port=8083;
char *rtcsp_pidfile="/var/run/"RTCSP_NAME".pid";

char *rtcsp_user="daemon";

int rtcsp_maxclients=1000;
int rtcsp_maxrecvs=2*1024*1024;

int server_start() {
	struct sockaddr_in sin;
	int listen_fd;
	int ret;

	int pid,i=21,cols=tput_cols();

	printf("Starting RtCSP server");
	strnprint(".",cols-i-9);
	flush();

	listen_fd=socket(AF_INET,SOCK_STREAM,0);
	if (listen_fd<0) {
		system("echo -e \"\\E[31m\".[Failed]");
		system("tput sgr0");
		printf("Not on the host %s bind port %d\n",rtcsp_host,rtcsp_port);
		return 0;
	}

	socket_opt(listen_fd);

	bzero(&sin,sizeof(sin));
	sin.sin_family=AF_INET;
	sin.sin_addr.s_addr=inet_addr(rtcsp_host);
	sin.sin_port=htons(rtcsp_port);

	ret = bind(listen_fd, (struct sockaddr *)&sin, sizeof(sin));
	if (ret<0) {
		system("echo -e \"\\E[31m\".[Failed]");
		system("tput sgr0");
		printf("Not on the host %s bind port %d\n", rtcsp_host, rtcsp_port);
		return 0;
	}

	ret=listen(listen_fd, rtcsp_backlog);
	if (ret<0) {
		system("echo -e \"\\E[31m\".[Failed]");
		system("tput sgr0");
		return 0;
	}

	pid=fork();

	if (pid==-1) {
		system("echo -e \"\\E[31m\".[Failed]");
		system("tput sgr0");
		printf("fork failure!\n");
		return 0;
	}
	if (pid>0) {
		FILE *fp;
		fp=fopen(rtcsp_pidfile,"w+");
		if (fp==NULL) {
			printf("file '%s' open fail.\n",rtcsp_pidfile);
		} else {
			fprintf(fp,"%d",pid);
			fclose(fp);
		}
		sleep(1);
		return 1;
	}

	struct passwd *pwnam;
	pwnam = getpwnam(rtcsp_user);

	if(pwnam) {
		setuid(pwnam->pw_uid);
		setgid(pwnam->pw_gid);
	} else {
		printf("Not found user %s.\n", rtcsp_user);
	}

	ret=setsid();
	if (ret<1) {
		system("echo -e \"\\E[31m\".[Failed]");
		system("tput sgr0");
	} else {
		system("echo -e \"\\E[32m\"[Succeed]");
		system("tput sgr0");
	}

	loop_event(listen_fd);

	exit(0);
}

int server_stop()
{
	FILE *fp;
	int pid,i=21,cols=tput_cols();

	printf("Stopping RtCSP server");
	flush();

	fp=fopen(rtcsp_pidfile,"r+");
	if (fp!=NULL) {
		fscanf(fp,"%d",&pid);
		fclose(fp);
		unlink(rtcsp_pidfile);
		if (pid==getsid(pid)) {
			kill(pid,SIGINT);
			while (pid==getsid(pid)) {
				printf(".");
				flush();
				i++;
				sleep(1);
				if (cols-i-9==0) {
					kill(pid,SIGKILL);
					break;
				}
			}
			strnprint(".",cols-i-9);
			flush();
			system("echo -e \"\\E[32m\"[Succeed]");
			system("tput sgr0");
			return 1;
		}
	}
	strnprint(".",cols-i-8);
	flush();
	system("echo -e \"\\E[31m\"[Failed]");
	system("tput sgr0");
	return 0;
}

int server_status() {
	FILE *fp;
	int pid,i=19,cols=tput_cols();

	printf("RtCSP server status");
	strnprint(".",cols-i-9);
	flush();

	fp=fopen(rtcsp_pidfile,"r+");
	if (fp!=NULL) {
		fscanf(fp,"%d",&pid);
		fclose(fp);
		if (pid==getsid(pid)) {
			system("echo -e \"\\E[32m\"[Running]");
			system("tput sgr0");
			return 1;
		}
		unlink(rtcsp_pidfile);
	}
	system("echo -e \"\\E[31m\"[stopped]");
	system("tput sgr0");
	return 0;
}
