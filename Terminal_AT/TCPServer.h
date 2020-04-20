#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

#define LISTENQ     1           //这个程序中只接纳一个Client
#define MAXCONN     60000

#define TCP_LOCAL_IP "128.0.1.1"
#define TCP_LOCAL_PORT 4000

void* TCP_thread(void *arg);
int TcpSocketInit(const char *ip, const int port);
void handle_accpet(int epollfd, int listenfd);

#endif