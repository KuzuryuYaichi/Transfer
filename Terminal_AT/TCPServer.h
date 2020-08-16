#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

#define LISTENQ 1           //这个程序中只接纳一个Client
#define MAXCONN 60000

#define TCP_LOCAL_IP "192.168.1.221"
#define TCP_LOCAL_PORT_CC 4001
#define TCP_LOCAL_PORT_FC 28672

void* TCP_thread(void *arg);
int TcpSocketInit(const char *ip, const int port);
int handle_accept(int epollfd, int listenfd);

#endif