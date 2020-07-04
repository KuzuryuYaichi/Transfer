#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include "UDP.h"
#include "TCPServer.h"
#include "Terminal.h"
#include "TCPServer.h"
#include "Monitor.h"

/*                Tcp2Udp_Queue              
                  ============>               
   TCP_Deal_Func     Monitor    UDP_Deal_Func 
                  <============               
                  Udp2Tcp_Queue               */

int Tcp2Udp_qid = 0, Udp2Tcp_qid = 0;
int Server_time = 0;

void t_daemon(char* file)
{
    const char *w_buf = "WM";
    if(access(file,F_OK))
        mkfifo(file,0777);
    int fd;
    if((fd=open(file,O_WRONLY|O_NONBLOCK,0)) > 0)
    {
        write(fd,w_buf,strlen(w_buf));
        close(fd);
    }
}

void HeartAlarmHandler(int i)
{
    alarm(10);
}

void initEnv()
{
    signal(SIGCHLD, SIG_DFL);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, HeartAlarmHandler);
}

void initQueue()
{
    while(Tcp2Udp_qid<=0)
    {
        Tcp2Udp_qid = msgget(IPC_PRIVATE,IPC_CREAT|0666);
        if(Tcp2Udp_qid<=0)printf("creat Tcp2Udp queue failed !\r\n");

        struct timeval timeout={0,10000};
        select(0,NULL,NULL,NULL,&timeout);
    }
    printf("Tcp2Udp_qid is : %d \r\n",Tcp2Udp_qid);

    while(Udp2Tcp_qid<=0)
    {
        Udp2Tcp_qid = msgget(IPC_PRIVATE,IPC_CREAT|0666);
        if(Udp2Tcp_qid<=0)printf("creat Udp2Tcp queue failed !\r\n");

        struct timeval timeout={0,10000};
        select(0,NULL,NULL,NULL,&timeout);
    }
    printf("Udp2Tcp_qid is : %d \r\n",Udp2Tcp_qid);

    printf("t2 pid is:%d\r\n",getpid());
}

void main_loop()
{
    // pthread_t TCP_tid, UDP_tid;
    // if(pthread_create(&TCP_tid,NULL,TCP_thread,NULL))
    // {
    //     perror("Create TCP thread error!");
    //     return;
    // }
    // if(pthread_create(&UDP_tid,NULL,UDP_thread,NULL))
    // {
    //     perror("Create UDP thread error!");
    //     return;
    // }
    // if(pthread_join(TCP_tid,NULL))
    // {
    //     perror("wait TCP thread error!!");
    //     return;
    // }
    // if(pthread_join(UDP_tid,NULL))
    // {
    //     perror("wait UDP thread error!!");
    //     return;
    // }
    // if(msgctl(Tcp2Udp_qid,IPC_RMID,0) < 0) perror("TCP2UDP IPC_MSG_QUEUE REMOVE FAILED\r\n");
    // if(msgctl(Udp2Tcp_qid,IPC_RMID,0) < 0) perror("UDP2TCP IPC_MSG_QUEUE REMOVE FAILED\r\n");

    pthread_t MON_tid;
    if(pthread_create(&MON_tid,NULL,MON_thread,NULL))
    {
        perror("Create MON thread error!");
        return;
    }
    if(pthread_join(MON_tid,NULL))
    {
        perror("wait MON thread error!!");
        return;
    } 
}

int main(int argc,char *argv[])
{
    pid_t pid = fork();
    if(pid < 0)
    {
        perror("fork error!");
        exit(1);
    }
    else if(pid > 0)
    {
        exit(0);
    }
    umask(0);
    pid_t sid = setsid();
    if (sid < 0)
    {
        return 0;
    }

    char szPath[128] = {0};
    if(getcwd(szPath,sizeof(szPath)))
    {
        chdir(szPath);
        printf("set current path succ [%s]\n",szPath);
    }
    else
    {
        perror("getcwd");
        exit(1);
    }
    
    printf("/**************************************APP**************************************/\r\n");
    initEnv();
    initQueue();
    main_loop();
    printf("Exit t2 Program\r\n");
    return 0;
}
