#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/msg.h>
#include "EpollEvent.h"
#include "TypeDefine.h"
#include "UDP.h"

static bool isRunning = true;

void do_epoll(int listenfd_CC, int listenfd_FC, int udpfd, HANDLE_EVENT FunEventPtr)
{
    struct epoll_event events[EPOLLEVENTS];
    int ret;
    //创建一个描述符
    int epollfd = epoll_create(FDSIZE);
    //添加监听描述符事件
    add_event(epollfd, listenfd_CC, EPOLLIN);
    add_event(epollfd, listenfd_FC, EPOLLIN);
    add_event(epollfd, udpfd, EPOLLIN);
    while(isRunning)
    {
        //获取已经准备好的描述符事件
        ret = epoll_wait(epollfd,events,EPOLLEVENTS,-1);
        FunEventPtr(epollfd,events,ret);
    }
    printf("epoll exit\r\n");
    close(epollfd);
}

void add_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}

void delete_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
}

void modify_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}

void do_read_tcp(int epollfd, int fd, int oppofd, int qid)
{
    printf("TCP Read qid is %d\r\n", qid);
    char *buf = (char*)malloc(MAXSIZE);
    int nread = recv(fd, buf, MAXSIZE, 0);
    if (nread == -1)
    {
        perror("read error:");
        close(fd);
        delete_event(epollfd, fd, EPOLLIN);
        free(buf);
    }
    else if (nread == 0)
    {
        perror("client close");
        close(fd);
        delete_event(epollfd, fd, EPOLLIN);
        free(buf);
    }
    else if (nread > 0)
    {
        struct Terminal_MsgSt msg = {0};
        msg.MsgType = IPC_NOWAIT;
        msg.msg = (struct TransMessage*)malloc(sizeof(struct TransMessage));
        msg.msg->data = buf;
        msg.msg->length = MAXSIZE;
        if(msgsnd(qid, &msg, sizeof(msg.msg), IPC_NOWAIT))//消息队列发送失败
        {
            printf("Send queue failed\r\n");
            free(msg.msg->data);
            free(msg.msg);
        }
        //修改描述符对应的事件，由读改为写
        modify_event(epollfd, oppofd, EPOLLOUT);
    }
}

void do_write_tcp(int epollfd, int fd, int oppofd, int qid)
{
    struct Terminal_MsgSt msg = {0};
    msg.MsgType = IPC_NOWAIT;
    while(msgrcv(qid, &msg, sizeof(msg.msg), 0, IPC_NOWAIT) > 0)
    {
        printf("TCP Write, qid is %d\r\n", qid);
        printf("msgrcv data : %p, length : %d\r\n", msg.msg->data, msg.msg->length);
        int nwrite = send(fd, msg.msg->data, msg.msg->length, 0);
        if (nwrite == -1)
        {
            perror("Tcp Write Error:");
            close(fd);
            delete_event(epollfd, fd, EPOLLOUT);
        }
        else
            modify_event(epollfd, fd, EPOLLIN);
        free(msg.msg->data);
        free(msg.msg);
    }
}

void do_read_udp(int epollfd, int fd, int oppofd_CC, int oppofd_FC, int qid_CC, int qid_FC)
{
    struct sockaddr_in remoteAddr;
    socklen_t len = sizeof(remoteAddr);
    printf("UDP Read\r\n");
    char *buf = (char*)malloc(MAXSIZE);
    int nread = recvfrom(fd, buf, MAXSIZE, 0, (struct sockaddr*)&remoteAddr, &len);
    char remoteIP[20];
    inet_ntop(AF_INET, &remoteAddr.sin_addr, remoteIP, sizeof(remoteIP));
    int remotePort = ntohs(remoteAddr.sin_port);
    if (nread == -1)
    {
        perror("read error:");
        close(fd);
        delete_event(epollfd, fd, EPOLLIN);
        free(buf);
    }
    else if (nread == 0)
    {
        perror("client close");
        close(fd);
        delete_event(epollfd, fd, EPOLLIN);
        free(buf);
    }
    else if (nread > 0)
    {
        struct Terminal_MsgSt msg = {0};
        msg.MsgType = IPC_NOWAIT;
        msg.msg = (struct TransMessage*)malloc(sizeof(struct TransMessage));
        msg.msg->data = buf;
        msg.msg->length = MAXSIZE;
        //修改描述符对应的事件，由读改为写
        int oppofd = -1, qid = -1;
        if(strcmp(remoteIP, UDP_REMOTE_IP))
            return;                                                             
        // if(remotePort == UDP_REMOTE_PORT_CC)
        // {
            qid = qid_CC;
            oppofd = oppofd_CC;
            printf("qid_CC is %d\r\n", qid);
        // }
        // else if(remotePort == UDP_REMOTE_PORT_FC)
        // {
        //     qid = qid_FC;
        //     oppofd = oppofd_FC;
        //     printf("UDP Read, qid_FC is %d\r\n", qid);
        // }
        if(msgsnd(qid, &msg, sizeof(msg.msg), IPC_NOWAIT))//消息队列发送失败
        {
            printf("Send queue failed\r\n");
            free(msg.msg->data);
            free(msg.msg);
        }
        modify_event(epollfd, oppofd, EPOLLOUT);
    }
}

void do_write_udp(int epollfd, int fd, int qid_CC, int qid_FC, struct sockaddr_in *remoteAddr_CC, struct sockaddr_in *remoteAddr_FC)
{
    socklen_t len = sizeof(struct sockaddr_in);
    struct Terminal_MsgSt msg = {0};
    msg.MsgType = IPC_NOWAIT;
    while(msgrcv(qid_CC, &msg, sizeof(msg.msg), 0, IPC_NOWAIT) > 0)
    {
        printf("UDP Write\r\n");
        printf("msgrcv data : %p, length : %d\r\n", msg.msg->data, msg.msg->length);
        int nwrite = sendto(fd, msg.msg->data, msg.msg->length, 0, (struct sockaddr*)remoteAddr_CC, len);
        if (nwrite == -1)
        {
            perror("Udp Write Error:");
            close(fd);
            delete_event(epollfd, fd, EPOLLOUT);
        }
        else
            modify_event(epollfd, fd, EPOLLIN);

        printf("qid_CC is %d\r\n", qid_CC);

        free(msg.msg->data);
        free(msg.msg);
    }
    while(msgrcv(qid_FC, &msg, sizeof(msg.msg), 0, IPC_NOWAIT) > 0)
    {
        printf("UDP Write\r\n");
        printf("msgrcv data : %p, length : %d\r\n", msg.msg->data, msg.msg->length);
        int nwrite = sendto(fd, msg.msg->data, msg.msg->length, 0, (struct sockaddr*)remoteAddr_FC, len);
        if (nwrite == -1)
        {
            perror("Udp Write Error:");
            close(fd);
            delete_event(epollfd, fd, EPOLLOUT);
        }
        else
            modify_event(epollfd, fd, EPOLLIN);

        printf("qid_FC is %d\r\n", qid_FC);

        free(msg.msg->data);
        free(msg.msg);
    }
}