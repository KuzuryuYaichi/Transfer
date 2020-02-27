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
#include "TCPServer.h"
#include "typeDefine.h"
#include "EpollEvent.h"

extern int Tcp2Udp_qid, Udp2Tcp_qid;

static bool isRunning = true;

void* TCP_thread(void *arg)
{
    int listen_fd = SocketInit("", LOCAL_PORT);
    listen(listen_fd,LISTENQ);
    while(isRunning)
    {
        t_daemon("tcpfile");
        do_epoll(listen_fd, handle_events);
    }
    close(listen_fd);
}

static int SocketInit(const char* ip,int port)
{
    struct sockaddr_in servaddr;
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
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
    //inet_pton(AF_INET,ip,&servaddr.sin_addr);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);//绑定所有网卡所有IP
    //servaddr.sin_addr.s_addr = inet_addr("172.16.6.178");
    //servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");//这样写指代不明，当服务器有多网卡时，不知道绑定哪个IP，导致连接失败
    servaddr.sin_port = htons(port);
    if (bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
    {
        perror("bind error: ");
        exit(1);
    }
    printf("listen on: %d, listenfd = %d\n",PORT,listenfd);
    return listenfd;
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
        if (fd == listenfd && (events[i].events & EPOLLIN))
            handle_accpet(epollfd, listenfd);
        else if (events[i].events & EPOLLIN)
            do_read(epollfd, fd, Tcp2Udp_qid);
        else if (events[i].events & EPOLLOUT)
            do_write(epollfd, fd, Udp2Tcp_qid);
    }
}
static void handle_accpet(int epollfd, int listenfd)
{
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen = sizeof(cliaddr);
    int clifd = accept(listenfd,(struct sockaddr*)&cliaddr,&cliaddrlen);
    if (clifd == -1)
        perror("accpet error:");
    else
    {
        printf("accept a new client: %s:%d,fd=%d\n",inet_ntoa(cliaddr.sin_addr),cliaddr.sin_port,clifd);
        //添加一个客户描述符和事件
        add_event(epollfd,clifd,EPOLLIN);
    }
}
static void do_read(int epollfd, int fd, int qid)
{
    char *buf = (char*)malloc(MAXSIZE);
    int nread = read(fd, buf, MAXSIZE);
    if (nread == -1)
    {
        perror("read error:");
        close(fd);
        delete_event(epollfd, fd, EPOLLIN);
    }
    else if (nread == 0)
    {
        fprintf(stderr,"client close,fd=%d\n", fd);
        close(fd);
        delete_event(epollfd, fd, EPOLLIN);
    }
    else
    {
        printf("read message is: %s,fd=%d\n", buf, fd);
        //修改描述符对应的事件，由读改为写
        modify_event(epollfd, fd, EPOLLOUT);
    }
}
static void do_write(int epollfd, int fd, int qid)
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