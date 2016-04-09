#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <math.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>

#include "setupconn.h"

/**********************************************************************/
int timeout_connect(int fd, const char *hostname, int port, int mstimeout) {
    int ret = 0;
    int flags;
    fd_set fds;
    struct timeval tv;
    struct addrinfo *res=NULL;
    struct addrinfo hints;
    char sport[BUFLEN];
    int err;

    hints.ai_flags          = 0;
    hints.ai_family         = AF_INET;
    hints.ai_socktype       = SOCK_STREAM;
    hints.ai_protocol       = IPPROTO_TCP;
    hints.ai_addrlen        = 0;
    hints.ai_addr           = NULL;
    hints.ai_canonname      = NULL;
    hints.ai_next           = NULL;

    snprintf(sport,BUFLEN,"%d",port);

    err = getaddrinfo(hostname,sport,&hints,&res);
    if(err|| (res==NULL))
    {
		if(res)
			freeaddrinfo(res);
		return -1;
    }
    // set non blocking
    if((flags = fcntl(fd, F_GETFL)) < 0) {
		fprintf(stderr, "timeout_connect: unable to get socket flags\n");
		freeaddrinfo(res);
		return -1;
    }
    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
		fprintf(stderr, "timeout_connect: unable to put the socket in non-blocking mode\n");
		freeaddrinfo(res);
		return -1;
    }
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    if(mstimeout >= 0) 
    {
		tv.tv_sec = mstimeout / 1000;
		tv.tv_usec = (mstimeout % 1000) * 1000;

		errno = 0;

		if(connect(fd, res->ai_addr, res->ai_addrlen) < 0) 
		{
			if((errno != EWOULDBLOCK) && (errno != EINPROGRESS))
			{
				fprintf(stderr, "timeout_connect: error connecting: %d\n", errno);
				freeaddrinfo(res);
				return -1;
			}
		}
		ret = select(fd+1, NULL, &fds, NULL, &tv);
    }
    freeaddrinfo(res);

    if(ret != 1) 
    {
		if(ret == 0)
			return -1;
		else
			return ret;
    }
    return 0;
}
/**********************************************************************/
int make_tcp_connection(const char *hostname, unsigned short port)
{
    struct sockaddr_in local;
    int s;
    int err;
    int mstimeout = 3000;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if(s<0) {
		perror("make_tcp_connection: socket");
		exit(1);
    }
    local.sin_family = PF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(INADDR_ANY);

    err=bind(s,(struct sockaddr *)&local, sizeof(local));
    if(err)
    {
		perror("make_tcp_connection_from_port::bind");
		return -4;
    }

    err = timeout_connect(s,hostname,port, mstimeout);
    if(err)
    {
		perror("make_tcp_connection: connect");
		close(s);
		return err; // bad connect
    }
    return s;
}
	
