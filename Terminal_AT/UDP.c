#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "UDP.h"
#include "TypeDefine.h"
#include "EpollEvent.h"

extern int Tcp2Udp_qid, Udp2Tcp_qid;

struct sockaddr_in remoteAddr_CC, remoteAddr_FC;

int UdpSocketInit(const char *localIP, const int localPort, const char *remoteIP, const int remotePort_CC, const int remotePort_FC)
{
    bzero(&remoteAddr_CC,sizeof(remoteAddr_CC));
    remoteAddr_CC.sin_family = AF_INET;
    remoteAddr_CC.sin_addr.s_addr = inet_addr(remoteIP);
    remoteAddr_CC.sin_port = htons(remotePort_CC);
    
    bzero(&remoteAddr_FC,sizeof(remoteAddr_FC));
    remoteAddr_FC.sin_family = AF_INET;
    remoteAddr_FC.sin_addr.s_addr = inet_addr(remoteIP);
    remoteAddr_FC.sin_port = htons(remotePort_FC);

    int udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpfd == -1)
    {
        perror("socket error:");
        exit(1);
    }
    // 一个端口释放后会等待两分钟之后才能再被使用 SO_REUSEADDR是让端口释放后立即就可以被再次使用
    int reuse_addr = 1;
    if (setsockopt(udpfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr)) == -1)
    {
        perror("Udp Reuse Error:");
        return -1;
    }
    struct sockaddr_in servaddr;
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    // servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 绑定所有网卡所有IP
    servaddr.sin_addr.s_addr = inet_addr(localIP);
    servaddr.sin_port = htons(localPort);
    if (bind(udpfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
    {
        perror("UDP bind error");
        exit(1);
    }
    printf("udp created on %s:%d, udpfd = %d\r\n", localIP, localPort, udpfd);
    return udpfd;
}