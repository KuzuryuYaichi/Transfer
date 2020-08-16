#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "TCPServer.h"
#include "UDP.h"
#include "EpollEvent.h"

int listenfd_CC, listenfd_FC, clifd_CC, clifd_FC, udpfd;

extern int Tcp2Udp_qid_CC, Udp2Tcp_qid_CC, Tcp2Udp_qid_FC, Udp2Tcp_qid_FC;

extern struct sockaddr_in remoteAddr_CC, remoteAddr_FC;

static void handle_events(int epollfd, struct epoll_event *events, int num)
{
    //进行选好遍历
    for (int i=0; i<num; i++)
    {
        int fd = events[i].data.fd;
        uint32_t event = events[i].events;
        //根据描述符的类型和事件类型进行处理
        if (fd == listenfd_CC && (event & EPOLLIN))
            clifd_CC = handle_accept(epollfd, listenfd_CC);
        else if(fd == listenfd_FC && (event & EPOLLIN))
            clifd_FC = handle_accept(epollfd, listenfd_FC);
        else
        {
            if (fd == udpfd)
            {
                if (event & EPOLLIN)
                    do_read_udp(epollfd, fd, clifd_CC, clifd_FC, Udp2Tcp_qid_CC, Udp2Tcp_qid_FC);
                else if (event & EPOLLOUT)
                    do_write_udp(epollfd, fd, Tcp2Udp_qid_CC, Tcp2Udp_qid_FC, &remoteAddr_CC, &remoteAddr_FC);
            }
            else if (fd == clifd_CC)
            {
                if (event & EPOLLIN)
                    do_read_tcp(epollfd, fd, udpfd, Tcp2Udp_qid_CC);
                else if (event & EPOLLOUT)
                    do_write_tcp(epollfd, fd, udpfd, Udp2Tcp_qid_CC);
            }
            else if (fd == clifd_FC)
            {
                if (event & EPOLLIN)
                    do_read_tcp(epollfd, fd, udpfd, Tcp2Udp_qid_FC);
                else if (event & EPOLLOUT)
                    do_write_tcp(epollfd, fd, udpfd, Udp2Tcp_qid_FC);
            }
        }
    }
}

void* MON_thread(void *arg)
{
    listenfd_CC = TcpSocketInit(TCP_LOCAL_IP, TCP_LOCAL_PORT_CC);
    listenfd_FC = TcpSocketInit(TCP_LOCAL_IP, TCP_LOCAL_PORT_FC);
    udpfd = UdpSocketInit(UDP_LOCAL_IP, UDP_LOCAL_PORT, UDP_REMOTE_IP, UDP_REMOTE_PORT_CC, UDP_REMOTE_PORT_FC);
    listen(listenfd_CC,LISTENQ);
    listen(listenfd_FC,LISTENQ);
    do_epoll(listenfd_CC, listenfd_FC, udpfd, handle_events);
    close(udpfd);
    close(clifd_CC);
    close(clifd_FC);
    close(listenfd_CC);
    close(listenfd_FC);
}