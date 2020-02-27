#ifndef _UDP_TERMINAL_H_
#define _UDP_TERMINAL_H_

#define REMOTE_IP "192.168.1.3"
#define REMOTE_PORT 8080
#define LOCAL_IP INADDR_ANY
#define LOCAL_PORT 8080

#define MAXSIZE 1024

int UDP0_init(void);
int UpdataSend(int sockfd);
int RecvUpdata(int sockfd);

#endif
