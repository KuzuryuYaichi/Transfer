#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
 
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/msg.h>
 
#include <sys/resource.h>
#include <signal.h>
#include <fcntl.h>
#include "UDP.h"
#include "TypeDefine.h"
#include "EpollEvent.h"

extern int Tcp2Udp_qid, Udp2Tcp_qid;

static int SocketInit(const char* ip, int port)
{
    struct sockaddr_in servaddr;
    int udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpfd == -1)
    {
        perror("socket error:");
        exit(1);
    }
    //一个端口释放后会等待两分钟之后才能再被使用，SO_REUSEADDR是让端口释放后立即就可以被再次使用。
    int reuse_addr = 1;
    if (setsockopt(udpfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) == -1)
    {
        return -1;
    }
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    //inet_pton(AF_INET,ip,&servaddr.sin_addr);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);//绑定所有网卡所有IP
    //servaddr.sin_addr.s_addr = inet_addr("172.16.6.178");
    //servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");//这样写指代不明，当服务器有多网卡时，不知道绑定哪个IP，导致连接失败
    servaddr.sin_port = htons(port);
    if (bind(udpfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
    {
        perror("bind error: ");
        exit(1);
    }
    printf("udp socket port : %d, fd=%d\n",port, udpfd);
    return udpfd;
}

static void handle_events(int epollfd, struct epoll_event *events, int num, int listenfd)
{
    int i;
    int fd;
    //进行选好遍历
    for (i=0; i<num; i++)
    {
        fd = events[i].data.fd;
        //根据描述符的类型和事件类型进行处理
        if (events[i].events & EPOLLIN)
            do_read(epollfd,fd,Udp2Tcp_qid);
        else if (events[i].events & EPOLLOUT)
            do_write(epollfd,fd,Tcp2Udp_qid);
    }
}

void* UDP_thread(void *arg)
{
    int udp_fd = SocketInit("", UDP_PORT);
    do_epoll(udp_fd, handle_events);
    close(udp_fd);
}