#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "UDP.h"
#include "TypeDefine.h"
#include "EpollEvent.h"

int udpfd;

extern int Tcp2Udp_qid, Udp2Tcp_qid;

struct sockaddr_in remoteAddr;

int UdpSocketInit(const char *localIP, const int localPort, const char *remoteIP, const int remotePort)
{
    printf("UDP Init Start\r\n");
    bzero(&remoteAddr,sizeof(remoteAddr));
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_addr.s_addr = inet_addr(remoteIP);
    remoteAddr.sin_port = htons(remotePort);
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpfd == -1)
    {
        perror("socket error:");
        exit(1);
    }
    //一个端口释放后会等待两分钟之后才能再被使用，SO_REUSEADDR是让端口释放后立即就可以被再次使用。
    int reuse_addr = 1;
    if (setsockopt(udpfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) == -1)
    {
        perror("Udp Reuse Error:");
        return -1;
    }
    struct sockaddr_in servaddr;
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);//绑定所有网卡所有IP
    // servaddr.sin_addr.s_addr = inet_addr(localIP);
    servaddr.sin_port = htons(localPort);
    if (bind(udpfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
    {
        perror("UDP bind error");
        exit(1);
    }
    printf("udp created on %s:%d, udpfd = %d\r\n", localIP, localPort, udpfd);
    return udpfd;
}

// static void handle_events(int epollfd, struct epoll_event *events, int num, int listenfd)
// {
//     int i;
//     //进行选好遍历
//     for (i=0; i<num; i++)
//     {
//         int fd = events[i].data.fd;
//         //根据描述符的类型和事件类型进行处理
//         if (events[i].events & EPOLLIN)
//             do_read(epollfd,fd,Udp2Tcp_qid);
//         else if (events[i].events & EPOLLOUT)
//             do_write(epollfd,fd,Tcp2Udp_qid);
//     }
// }

// void* UDP_thread(void *arg)
// {
//     int udp_fd = SocketInit("", UDP_PORT);
//     do_epoll(udp_fd, handle_events);
//     close(udp_fd);
// }