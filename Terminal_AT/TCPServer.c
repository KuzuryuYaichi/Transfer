#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "TCPServer.h"
#include "TypeDefine.h"
#include "EpollEvent.h"

int listenfd, clifd;

extern int Tcp2Udp_qid, Udp2Tcp_qid;

int TcpSocketInit(const char *localIP, const int localPort)
{
    struct sockaddr_in servaddr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
    {
        perror("socket error:");
        exit(1);
    }
    //一个端口释放后会等待两分钟之后才能再被使用，SO_REUSEADDR是让端口释放后立即就可以被再次使用。
    int reuse_addr = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) == -1)
    {
        return -1;
    }
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    // servaddr.sin_addr.s_addr = htonl(INADDR_ANY);//绑定所有网卡所有IP
    servaddr.sin_addr.s_addr = inet_addr(localIP);
    servaddr.sin_port = htons(localPort);
    if (bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
    {
        perror("TCP bind error");
        exit(1);
    }
    printf("listen on %s:%d, listenfd = %d\n", localIP, localPort, listenfd);
    return listenfd;
}

void handle_accpet(int epollfd, int listenfd)
{
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen = sizeof(cliaddr);
    clifd = accept(listenfd,(struct sockaddr*)&cliaddr,&cliaddrlen);
    if (clifd == -1)
        perror("accpet error:");
    else
    {
        printf("accept a new client: %s:%d, fd = %d\n", inet_ntoa(cliaddr.sin_addr),cliaddr.sin_port,clifd);
        //添加一个客户描述符和事件
        add_event(epollfd,clifd,EPOLLIN);
    }
}

// static void handle_events(int epollfd, struct epoll_event *events, int num, int listenfd)
// {
//     int i;
//     //进行选好遍历
//     for (i=0; i<num; i++)
//     {
//         int fd = events[i].data.fd;
//         //根据描述符的类型和事件类型进行处理
//         if (fd == listenfd && (events[i].events & EPOLLIN))
//             handle_accpet(epollfd, listenfd);
//         else if (events[i].events & EPOLLIN)
//             do_read(epollfd, fd, Tcp2Udp_qid);
//         else if (events[i].events & EPOLLOUT)
//             do_write(epollfd, fd, Udp2Tcp_qid);
//     }
// }

// void* TCP_thread(void *arg)
// {
//     int listen_fd = SocketInit("", TCP_PORT);
//     listen(listen_fd,LISTENQ);
//     do_epoll(listen_fd, handle_events);
//     close(listen_fd);
// }
