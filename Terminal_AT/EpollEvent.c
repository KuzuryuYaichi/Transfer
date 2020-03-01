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
#include "EpollEvent.h"
#include "TypeDefine.h"

static bool isRunning = true;

void do_epoll(int listenfd, HANDLE_EVENT FunEventPtr)
{
    struct epoll_event events[EPOLLEVENTS];
    int ret;
    //创建一个描述符
    int epollfd = epoll_create(FDSIZE);
    //添加监听描述符事件
    add_event(epollfd,listenfd,EPOLLIN);
    while(isRunning)
    {
        //获取已经准备好的描述符事件
        ret = epoll_wait(epollfd,events,EPOLLEVENTS,-1);
        FunEventPtr(epollfd,events,ret,listenfd);
    }
    close(epollfd);
}
void add_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}
static void delete_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
}
static void modify_event(int epollfd, int fd, int state)
{
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}

void do_read(int epollfd, int fd, int qid)
{
    char *buf = (char*)malloc(MAXSIZE);
    int nread = read(fd, buf, MAXSIZE);
    if (nread == -1)
    {
        perror("read error:");
        close(fd);
        delete_event(epollfd, fd, EPOLLIN);
        free(buf);
    }
    else if (nread == 0)
    {
        fprintf(stderr,"client close,fd=%d\n", fd);
        close(fd);
        delete_event(epollfd, fd, EPOLLIN);
        free(buf);
    }
    else
    {
        printf("read message is: %s,fd=%d\n", buf, fd);
        //修改描述符对应的事件，由读改为写
        // modify_event(epollfd, fd, EPOLLOUT);
        struct Terminal_MsgSt msg = {0};
        msg.MsgType = IPC_NOWAIT;
        msg.msg->data = buf;
        msgsnd(qid, &msg, sizeof(msg.msg), IPC_NOWAIT);
    }
}

void do_write(int epollfd, int fd, int qid)
{
    struct Terminal_MsgSt msg = {0};
    msg.MsgType = IPC_NOWAIT;
    int result = msgrcv(qid, &msg, sizeof(msg.msg), 0, IPC_NOWAIT);
    if(result >= 0)
    {
        int nwrite = write(fd, msg.msg->data, msg.msg->length);
        if (nwrite == -1)
        {
            perror("write error:");
            close(fd);
            delete_event(epollfd, fd, EPOLLOUT);
        }
        else
            modify_event(epollfd, fd, EPOLLIN);
        free(msg.msg->data);
        free(msg.msg);
    }
}