#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <linux/types.h>
#include <getopt.h>
#include <libgen.h>
#include <signal.h>
#include <limits.h>
#include <net/if.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <math.h>
#include <linux/rtnetlink.h>
#include <linux/netlink.h>
#include <linux/watchdog.h>
#include <pthread.h>
#include <semaphore.h>
#include <syslog.h>
#include "UDP.h"
#include "TCP.h"
#include "GPIO.h"
#include "Terminal.h"

/*******            Tcp2Udp_Queue            ********
********            ============>            ********
******** TCP_Thread               UDP_Thread ********
********            <============            ********
********            Udp2Tcp_Queue            *******/

int Tcp2Udp_qid = 0, Udp2Tcp_qid = 0;
int Server_time = 0;

RUNNING_STATE isRunning = POWER_OFF, state = POWER_OFF;
INIT_AT ATState = NEED_TO_INIT;

void (*tcp_func)(int) = NULL;

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
    struct Terminal_MsgSt NetSend_queue = {0};
    NetSend_queue.MsgType = IPC_NOWAIT;
    NetSend_queue.MsgBuff[1] = HEART;
    msgsnd(CanRecv_qid, &NetSend_queue, 2, IPC_NOWAIT);
    alarm(10);
}

void prepare()
{
    signal(SIGPIPE,SIG_IGN);
    signal(SIGALRM,HeartAlarmHandler);

    initTerminal();
    ResetRecord();
    File_Configuration();
    init_mutex();
    
    while(Tcp2Udp_qid<=0)
    {
        Tcp2Udp_qid = msgget(IPC_PRIVATE,IPC_CREAT|0666);
        if(Tcp2Udp_qid<=0)printf("creat TCP queue failed !\r\n");

        struct timeval timeout={0,10000};
        select(0,NULL,NULL,NULL,&timeout);
    }
    printf("Tcp2Udp_qid is : %d \r\n",Tcp2Udp_qid);
    while(Udp2Tcp_qid<=0)
    {
        Udp2Tcp_qid = msgget(IPC_PRIVATE,IPC_CREAT|0666);
        if(Udp2Tcp_qid<=0)printf("creat UDP queue failed !\r\n");

        struct timeval timeout={0,10000};
        select(0,NULL,NULL,NULL,&timeout);
    }
    printf("Udp2Tcp_qid is : %d \r\n",Udp2Tcp_qid);

    printf("t2 pid is:%d\r\n",getpid());
}

void init_mutex()
{
    
}

void main_loop()
{
    pthread_t TCP_tid, UDP_tid;
    if(pthread_create(&TCP_tid,NULL,TCP_thread,NULL))
    {
        perror("Create TCP thread error!");
        return;
    }
    if(pthread_create(&UDP_tid,NULL,UDP_thread,NULL))
    {
        perror("Create UDP thread error!");
        return;
    }
    if(pthread_join(TCP_tid,NULL))
    {
        perror("wait TCP thread error!!");
        return;
    }
    if(pthread_join(UDP_tid,NULL))
    {
        perror("wait UDP thread error!!");
        return;
    }

    if(msgctl(TCP_tid,IPC_RMID,0)<0) perror("TCP IPC_MSG_QUEUE REMOVE FAILED\r\n");
    if(msgctl(UDP_tid,IPC_RMID,0)<0) perror("UDP IPC_MSG_QUEUE REMOVE FAILED\r\n");
}

int main(int argc,char *argv[])
{
    printf("/**************************************APP**************************************/\r\n");
    prepare();

    if(argc > 1)
    {
        isRunning = state = argv[2][0];
        if(state == PROGRAM_VIRTUAL_SERVER)
            ATState = argv[3][0];
        else if(state == PROGRAM_ACTUAL_SERVER)
            ATState = NEED_TO_INIT;
        printf("From:%s To:%s ATInit:%s Site_comm_token:%s\r\n",argv[1],argv[2],argv[3],argv[4]);
        main_loop();
    }
    else printf("�Ƿ�����ָ��\r\n");

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
    setsid();
    char szPath[128] = {0};
    if(getcwd(szPath,sizeof(szPath)) == NULL)
    {
        perror("getcwd");
        exit(1);
    }
    else
    {
        chdir(szPath);
        printf("set current path succ [%s]\n",szPath);
    }
    umask(0);
}
