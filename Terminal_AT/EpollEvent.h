#ifndef _EPOLL_EVENT_H_
#define _EPOLL_EVENT_H_

#define EPOLLEVENTS 60000
#define FDSIZE      1024
#define MAXSIZE     1100
#define bool int    //linux C中没有bool类型
#define false 0     //linux C中没有bool类型
#define true  1     //linux C中没有bool类型

typedef void (*HANDLE_EVENT)(int epollfd, struct epoll_event *events, int num, int listenfd);

//IO多路复用epoll
// void do_epoll(int listenfd, HANDLE_EVENT FunEventPtr);
void do_epoll(int listenfd, int udpfd, HANDLE_EVENT FunEventPtr);
//添加事件
void add_event(int epollfd, int fd, int state);
//修改事件
void modify_event(int epollfd, int fd, int state);
//删除事件
void delete_event(int epollfd, int fd, int state);
//读处理
void do_read_tcp(int epollfd, int fd, int oppofd, int qid);
//写处理
void do_write_tcp(int epollfd, int fd, int oppofd, int qid);
//读处理
void do_read_udp(int epollfd, int fd, int oppofd, int qid, struct sockaddr_in *remoteAddr);
//写处理
void do_write_udp(int epollfd, int fd, int oppofd, int qid, struct sockaddr_in *remoteAddr);

#endif