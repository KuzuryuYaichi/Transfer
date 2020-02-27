#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

#define IPADDRESS   "127.0.0.1"
#define PORT        1883
#define LISTENQ     1           //这个程序中只接纳一个Client
#define MAXCONN     60000
#define MAXSIZE     1024

#define LOCAL_IP INADDR_ANY
#define LOCAL_PORT 8080

//创建套接字并进行绑定
static int socket_bind(const char* ip,int port);

#endif