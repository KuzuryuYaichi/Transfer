#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/msg.h>
#include "EpollEvent.h"
#include "TypeDefine.h"

static bool isRunning = true;

// void do_epoll(int listenfd, HANDLE_EVENT FunEventPtr)
// {
//     struct epoll_event events[EPOLLEVENTS];
//     int ret;
//     //创建一个描述符
//     int epollfd = epoll_create(FDSIZE);
//     //添加监听描述符事件
//     add_event(epollfd,listenfd,EPOLLIN);
//     while(isRunning)
//     {
//         //获取已经准备好的描述符事件
//         ret = epoll_wait(epollfd,events,EPOLLEVENTS,-1);
//         FunEventPtr(epollfd,events,ret,listenfd);
//     }
//     printf("epoll exit\r\n");
//     close(epollfd);
// }

void do_epoll(int listenfd, int udpfd, HANDLE_EVENT FunEventPtr)
{
    struct epoll_event events[EPOLLEVENTS];
    int ret;
    //创建一个描述符
    int epollfd = epoll_create(FDSIZE);
    //添加监听描述符事件
    add_event(epollfd,listenfd,EPOLLIN);
    add_event(epollfd,udpfd,EPOLLIN);
    while(isRunning)
    {
        //获取已经准备好的描述符事件
        ret = epoll_wait(epollfd,events,EPOLLEVENTS,-1);
        FunEventPtr(epollfd,events,ret,listenfd);
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
    else
    {
        struct Terminal_MsgSt msg = {0};
        msg.MsgType = IPC_NOWAIT;
        msg.msg = (struct TransMessage*)malloc(sizeof(struct TransMessage));
        msg.msg->data = buf;
        msg.msg->length = MAXSIZE;
        if(msgsnd(qid, &msg, sizeof(msg.msg), IPC_NOWAIT) == 0)
            printf("Send queue successfully!\r\n");
        else
            printf("Send queue failed\r\n");
        //修改描述符对应的事件，由读改为写
        modify_event(epollfd, oppofd, EPOLLOUT);
    }
}

void do_write_tcp(int epollfd, int fd, int oppofd, int qid)
{
    printf("TCP Write, qid is %d\r\n", qid);
    struct Terminal_MsgSt msg = {0};
    msg.MsgType = IPC_NOWAIT;
    int result = msgrcv(qid, &msg, sizeof(msg.msg), 0, IPC_NOWAIT);
    if(result >= 0)
    {
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

void do_read_udp(int epollfd, int fd, int oppofd, int qid, struct sockaddr_in *remoteAddr)
{
    socklen_t len = sizeof(struct sockaddr_in);
    printf("UDP Read, qid is %d\r\n", qid);
    char *buf = (char*)malloc(MAXSIZE);
    int nread = recvfrom(fd, buf, MAXSIZE, 0, NULL, NULL);
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
    else
    {
        struct Terminal_MsgSt msg = {0};
        msg.MsgType = IPC_NOWAIT;
        msg.msg = (struct TransMessage*)malloc(sizeof(struct TransMessage));
        msg.msg->data = buf;
        msg.msg->length = MAXSIZE;
        if(msgsnd(qid, &msg, sizeof(msg.msg), IPC_NOWAIT) == 0)
            printf("Send queue successfully!\r\n");
        else
            printf("Send queue failed\r\n");
        //修改描述符对应的事件，由读改为写
        modify_event(epollfd, oppofd, EPOLLOUT);
    }
}

void do_write_udp(int epollfd, int fd, int oppofd, int qid, struct sockaddr_in *remoteAddr)
{
    socklen_t len = sizeof(struct sockaddr_in);
    printf("UDP Write, qid is %d\r\n", qid);
    struct Terminal_MsgSt msg = {0};
    msg.MsgType = IPC_NOWAIT;
    int result = msgrcv(qid, &msg, sizeof(msg.msg), 0, IPC_NOWAIT);
    if(result > 0)
    {
        printf("msgrcv data : %p, length : %d\r\n", msg.msg->data, msg.msg->length);
        int nwrite = sendto(fd, msg.msg->data, msg.msg->length, 0, (struct sockaddr*)remoteAddr, len);
        if (nwrite == -1)
        {
            perror("Udp Write Error:");
            close(fd);
            delete_event(epollfd, fd, EPOLLOUT);
        }
        else
            modify_event(epollfd, fd, EPOLLIN);
        free(msg.msg->data);
        free(msg.msg);
    }
}