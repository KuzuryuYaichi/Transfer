#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "TCPServer.h"
#include "UDP.h"
#include "EpollEvent.h"

extern int listenfd, clifd, udpfd;

extern int Tcp2Udp_qid, Udp2Tcp_qid;

extern struct sockaddr_in remoteAddr;

static void handle_events(int epollfd, struct epoll_event *events, int num, int listenfd)
{
    int i;
    //进行选好遍历
    for (i=0; i<num; i++)
    {
        int fd = events[i].data.fd;
        //根据描述符的类型和事件类型进行处理
        if (fd == listenfd && (events[i].events & EPOLLIN))
            handle_accpet(epollfd, listenfd);
        else
        {
            if (fd == clifd)
            {
                if (events[i].events & EPOLLIN)
                    do_read_tcp(epollfd, fd, udpfd, Tcp2Udp_qid);
                else if (events[i].events & EPOLLOUT)
                    do_write_tcp(epollfd, fd, udpfd, Udp2Tcp_qid);
            }
            else if (fd == udpfd)
            {
                if (events[i].events & EPOLLIN)
                    do_read_udp(epollfd, fd, clifd, Udp2Tcp_qid, &remoteAddr);
                else if (events[i].events & EPOLLOUT)
                    do_write_udp(epollfd, fd, clifd, Tcp2Udp_qid, &remoteAddr);
            }
        }
    }
}

void* MON_thread(void *arg)
{
    TcpSocketInit(TCP_LOCAL_IP, TCP_LOCAL_PORT);
    UdpSocketInit(UDP_LOCAL_IP, UDP_LOCAL_PORT, UDP_REMOTE_IP, UDP_REMOTE_PORT);
    listen(listenfd,LISTENQ);
    do_epoll(listenfd, udpfd, handle_events);
    close(udpfd);
    close(listenfd);
    close(clifd);
}