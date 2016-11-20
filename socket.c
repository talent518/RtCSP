#include <sys/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <assert.h>

#ifdef HAVE_RTCSP
	#include "RtCSP.h"
	#include "socket.h"
	#include "loop_event.h"
#else
	#include "bench.h"
#endif

int recv_data_len(conn_t *ptr)
{
	unsigned char buf[4];
	int len=0,i,ret;

	ret=recv(ptr->sockfd,buf,sizeof(buf), MSG_WAITALL);

	if(ret<0) {
		return -1;
	} else if(ret==0) {
		return 0;
	}

	if (ret!=sizeof(buf))
	{
		conn_info_ex(ptr, "[ The data packet is not complete ] ");
		return 0;
	}

	if (buf[0])
	{
		conn_info_ex(ptr, "[ The data packet head is not legitimate ] ");
		return 0;
	}

	for (i=0;i<4;i++)
	{
		len+=(buf[i]&0xff)<<((3-i)*8);
	}

	return len;
}

//接收来自客户端数据
//返回值:0(关闭连接),-1(接收到的数据长度与数据包长度不一致),>0(接收成功)
int socket_recv(conn_t *ptr,char **data,int *data_len)
{
	int ret;
	if(ptr->rbuf==NULL) {
		ret=recv_data_len(ptr);
		if (ret>0)
		{
			if (ret>rtcsp_maxrecvs)
			{
				conn_info_ex(ptr, "[ The received data beyond the limit ] ");
				return 0;
			}
			ptr->rbuf=(char*)malloc(ret+1);
			bzero(ptr->rbuf,ret+1);
			ptr->rsize=ret;
			ptr->rbytes=0;
		}
		else
		{
			return ret;
		}
	}
	
	ret=recv(ptr->sockfd,ptr->rbuf+ptr->rbytes,ptr->rsize-ptr->rbytes,MSG_DONTWAIT);

	if(ret<0) {
		return -1;
	} else if(ret==0) {
		return 0;
	}

	ptr->rbytes+=ret;

	if (ptr->rsize>0 && ptr->rbytes==ptr->rsize)
	{
		if (*data)
		{
			free(*data);
		}

		*data=ptr->rbuf;
		*data_len=ptr->rsize;

		ptr->rbuf=NULL;
		ptr->rsize=ptr->rbytes=0;

		return 1;
	}
	return -1;
}

int socket_send(conn_t *ptr,const char *data,int data_len)
{
	int i,ret,plen;
	char *package;

	if (data_len<=0)
	{
		return -1;
	}

	plen=4+data_len;
	package=(char*)malloc(plen);

	for (i=0;i<4;i++)
	{
		package[i]=data_len>>((3-i)*8);
	}

	memcpy(package+4,data,data_len);//数据包内容

	ret=send(ptr->sockfd,package,plen,MSG_WAITALL);
	free(package);
	
	if (ret>0 && ret!=plen)
	{
		conn_info_ex(ptr, "[ Failed sending data ] ");
	}

	return ret;
}

conn_t *socket_connect(const char *host, short int port){
	struct sockaddr_in sin;
	int sockfd;
	int ret;

	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd<0){
		return 0;
	}
	
	socket_opt(sockfd);

	bzero(&sin,sizeof(sin));
	sin.sin_family=AF_INET;
	sin.sin_addr.s_addr=inet_addr(host);
	sin.sin_port=htons(port);

	conn_t *ptr = NULL;
	if(connect(sockfd,(struct sockaddr *)&sin,sizeof(sin)) == 0) {
		ptr = (conn_t*)malloc(sizeof(conn_t));
		memset(ptr, 0, sizeof(conn_t));
		ptr->sockfd = sockfd;
		strcpy(ptr->host, host);
		ptr->port = port;
	}
	return ptr;
}

void socket_close(conn_t *ptr)
{
#ifdef HAVE_RTCSP
	char buf[1];
	buf[0] = '\0';

	BEGIN_READ_LOCK
	{
		if (ptr->refable)
		{
			ptr->refable=false;

			conn_info(ptr);

			queue_push(ptr->thread->close_queue, ptr);

			buf[0] = 'x';
		}
	} END_READ_LOCK;

	if(buf[0]) {
		write(ptr->thread->write_fd, buf, 1);
	}
#else
	if(ptr->event.ev_base) {
		event_del(&ptr->event);
	}

	shutdown(ptr->sockfd,SHUT_RDWR);
	close(ptr->sockfd);
	if(ptr->rbuf) {
		free(ptr->rbuf);
		ptr->rbuf = NULL;
	}
	ptr->rbytes = 0;
	ptr->rsize = 0;
#endif
}

void socket_opt(int sockfd) {
	int opt = 1, ret;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
	assert(ret == 0);
	ret = setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(int));
	assert(ret == 0);

#if !defined(__CYGWIN32__) && !defined(__CYGWIN__)
	struct timeval timeout = {3, 0};//3s
	ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));//发送超时
	assert(ret == 0);
	ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));//接收超时
	assert(ret == 0);
#else
	int send_timeout = 3000, recv_timeout = 3000;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(int));//发送超时
	assert(ret == 0);
	ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(int));//接收超时
	assert(ret == 0);
#endif

	typedef struct {
		int l_onoff;
		int l_linger;
	} linger;
	linger m_sLinger;
	m_sLinger.l_onoff=1;//(在closesocket()调用,但是还有数据没发送完毕的时候容许逗留)
	// 如果m_sLinger.l_onoff=0;则功能和2.)作用相同;
	m_sLinger.l_linger=5;//(容许逗留的时间为5秒)
	ret = setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (const char*)&m_sLinger, sizeof(linger));
	assert(ret == 0);

#if !defined(__CYGWIN32__) && !defined(__CYGWIN__)
	int send_recv_buffers = 16 * 1024;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&send_recv_buffers, sizeof(int));//发送缓冲区大小
	assert(ret == 0);
	ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&send_recv_buffers, sizeof(int));//接收缓冲区大小
	assert(ret == 0);
#endif
}
