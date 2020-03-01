#ifndef _EPOLL_EVENT_H_
#define _EPOLL_EVENT_H_

#define EPOLLEVENTS 60000
#define MAXSIZE     1024
#define FDSIZE      1024
#define bool int    //linux C中没有bool类型
#define false 0     //linux C中没有bool类型
#define true  1     //linux C中没有bool类型

typedef void (*HANDLE_EVENT)(int epollfd, struct epoll_event *events, int num, int listenfd);

//IO多路复用epoll
void do_epoll(int listenfd,  HANDLE_EVENT FunEventPtr);
//处理接收到的连接
static void handle_accpet(int epollfd,int listenfd);
//添加事件
void add_event(int epollfd,int fd,int state);
//修改事件
static void modify_event(int epollfd,int fd,int state);
//删除事件
static void delete_event(int epollfd,int fd,int state);
//读处理
void do_read(int epollfd,int fd,int qid);
//写处理
void do_write(int epollfd,int fd,int qid);

#endif