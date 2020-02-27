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
#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/rtnetlink.h>
#include <linux/netlink.h>
#include <linux/watchdog.h>
#include <pthread.h>
#include <semaphore.h>
#include <syslog.h>
#include "can.h"
#include "UDP.h"
#include "TCP.h"
#include "CRC.h"
#include "GPIO.h"
#include "Charger_can.h"
#include "Terminal.h"
#include "USART.h"

#define USART_BUFF_SIZE 255
#define TCP_MAX_LEN 255
#define BUF_LEN 1028

#define APP_FLAG 0x69153847

extern int CAN0_fd;      //CAN0
extern int USART_fd;     //ETH0(TCP)

int CanRecv_qid = 0;
int NetRecv_qid = 0;
int UpdataRecv_qid = 0;

struct Terminal_Info    Terminal = {0};
struct Knee_struct      Knee_can[EQUIP_NUM_MAX+1] = {0};      //���ڼ�¼׮��CAN��������
struct Net_struct       Net_data[EQUIP_NUM_MAX+1] = {0};      //���ڼ�¼��������������
struct UdpScan_struct   UpScan_Scan[EQUIP_NUM_MAX+1] = {0};
struct CheckKnee_Struct CheckKnee[EQUIP_NUM_MAX+1] = {0};     //���ڼ�¼��ѯ׮��Ϣȴ�����ص�׮��

__u8 SITE_state = NO_LOAD;                         //�����ն�״̬

int Green_light = LIGHT_OFF;
int Red_light = LIGHT_OFF;

RUNNING_STATE isRunning = PROGRAM_BOOTLOADER, state = PROGRAM_BOOTLOADER;
INIT_AT ATState = NEED_TO_INIT;

void (*tcp_func)(int) = NULL;

void t_daemon(char* file)
{
//    char *buf = "This is Terminal\n";
//    int fd_temp;
//    if((fd_temp = open(file,O_CREAT | O_RDWR,0600)) < 0)perror("open");
//    while(flock(fd_temp,LOCK_EX|LOCK_NB));
//    lseek(fd_temp,0,SEEK_SET);
//    if(write(fd_temp,buf,strlen(buf))!=strlen(buf))perror("write");
//    while(flock(fd_temp,LOCK_UN));
//    close(fd_temp);

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

void* LED_thread(void *arg)
{
    Green_light = LIGHT_ON;
    while(isRunning == state)
    {
        t_daemon("ledfile");
        switch(Green_light)
        {
            case LIGHT_OFF:
            {
                system("echo 0 > /sys/class/gpio/gpio195/value");
                break;
            }
            case LIGHT_ON:
            {
                system("echo 1 > /sys/class/gpio/gpio195/value");
                break;
            }
        }
        switch(Red_light)
        {
            case LIGHT_OFF:
            {
                system("echo 0 > /sys/class/gpio/gpio194/value");
                break;
            }
            case LIGHT_ON:
            {
                system("echo 1 > /sys/class/gpio/gpio194/value");
                break;
            }
        }
        if(Red_light == LIGHT_BLINK && Green_light == LIGHT_BLINK)
        {
            struct timeval tv1={0,300000};
            system("echo 1 > /sys/class/gpio/gpio195/value");
            system("echo 1 > /sys/class/gpio/gpio194/value");
            select(0,NULL,NULL,NULL,&tv1);
            struct timeval tv2={0,300000};
            system("echo 0 > /sys/class/gpio/gpio195/value");
            system("echo 0 > /sys/class/gpio/gpio194/value");
            select(0,NULL,NULL,NULL,&tv2);
        }
        else if(Red_light == LIGHT_BLINK && Green_light != LIGHT_BLINK)
        {
            struct timeval tv1={0,300000};
            system("echo 1 > /sys/class/gpio/gpio194/value");
            select(0,NULL,NULL,NULL,&tv1);
            struct timeval tv2={0,300000};
            system("echo 0 > /sys/class/gpio/gpio194/value");
            select(0,NULL,NULL,NULL,&tv2);
        }
        else if(Red_light != LIGHT_BLINK && Green_light == LIGHT_BLINK)
        {
            struct timeval tv1={0,300000};
            system("echo 1 > /sys/class/gpio/gpio195/value");
            select(0,NULL,NULL,NULL,&tv1);
            struct timeval tv2={0,300000};
            system("echo 0 > /sys/class/gpio/gpio195/value");
            select(0,NULL,NULL,NULL,&tv2);
        }
    }
}

void* CAN_thread(void *arg)
{
    while(CAN0_fd <= 0)
    {
        CAN0_fd = CAN_init();
        struct timeval timeout={0,100000}; // ����10ms���յȴ���ʱ
        select(0,NULL,NULL,NULL,&timeout);
    }
    printf("CAN0_fd is : %d\r\n",CAN0_fd);

    while(isRunning == state)
    {
        t_daemon("canfile");
        CanSendData(CAN0_fd);
        RecvCanData(CAN0_fd);     //����CAN��������
    }
}

void* NET_thread(void *arg)
{
    if(ATState == NEED_TO_INIT)
    {
        init_AT();
        int i=0;
        while(i++<3)
        {
            char data_send[50] = {0};
            if(Terminal.BackStage_Sel == 1)strcpy(data_send,"@BACKSTAGE,1\r\n");
            else strcpy(data_send,"@BACKSTAGE,2\r\n");
            printf("Write:%s",data_send);
            write(USART_fd,data_send,strlen(data_send));
            struct timeval tv={1,0};
            fd_set p_rd;
            FD_ZERO(&p_rd);
            FD_SET(USART_fd,&p_rd);
            if(FD_ISSET(USART_fd,&p_rd) && select(USART_fd + 1,&p_rd,NULL,NULL,&tv) > 0)
            {
                char buf[1024] = {0};
                if(read(USART_fd, buf, 1024)>0)
                {
                    printf("buf is %s\r\n",buf);
                    if(strstr(buf,"OK"))
                    {
                        printf("USART Init Success\r\n");
                        break;
                    }
                }
            }
        }
        if(i==4)printf("Connect to Server Failed\r\n");

        struct timeval tv={1,500000};
        fd_set p_rd;
        FD_ZERO(&p_rd);
        FD_SET(USART_fd,&p_rd);
        if(FD_ISSET(USART_fd,&p_rd) && select(USART_fd + 1,&p_rd,NULL,NULL,&tv) > 0)
        {
            usleep(1000);
            char buf[1024] = {0};
            if(read(USART_fd, buf, 1024)>0)
            {
                printf("buf is %s\r\n",buf);
                char *p = NULL;
                if(p = strstr(buf,"@CUSTOMERID,"))
                {
                    char data_send[50] = {0};
                    int customer_id = atoi(p + strlen("@CUSTOMERID,"));
                    if(Terminal.Customer_ID == 0 || customer_id == 1987 || Terminal.Customer_ID == customer_id)
                    {
                        if(Terminal.Customer_ID == 0 && customer_id != 1987)
                        {
                            site_id_reset(&customer_id,CUSTOMER_ID_WRITE);
                            File_Configuration();
                        }
                        strcpy(data_send,"OK,");
                    }
                    else
                    {
                        strcpy(data_send,"ERROR,");
                        isRunning == PROGRAM_ACTUAL_SERVER;
                    }
                    sprintf(data_send,"%s%04d\r\n",data_send,Terminal.Customer_ID);
                    printf("Write:%s",data_send);
                    write(USART_fd,data_send,strlen(data_send));
                }
            }
        }
        else isRunning == PROGRAM_ACTUAL_SERVER;
    }
    alarm(3);
    while(isRunning == state)
    {
        t_daemon("netfile");
        tcp_func(USART_fd);
    }
}

//��ʼ��������
void init_mutex()
{
    int i;
    for(i=0; i<=EQUIP_NUM_MAX; i++)
    {
        pthread_mutex_init(&Knee_can[i].mt,NULL);
        pthread_mutex_init(&Net_data[i].mt,NULL);
        pthread_mutex_init(&UpScan_Scan[i].mt,NULL);
    }
}

/*���������ļ�*/
int File_Configuration()
{
    cJSON *spec_data = NULL;
    USART_fd = USART_init();
    char szBuf[256] = {0};
    int fd;
    if((fd = open("/site_info",O_CREAT | O_RDWR,0600)) > 0)
    {
        while(flock(fd,LOCK_EX|LOCK_NB));
        lseek(fd,0,SEEK_SET);
        if(read(fd,szBuf,sizeof(szBuf)) == -1)perror("read");
        while(flock(fd,LOCK_UN));
        close(fd);

        spec_data = cJSON_Parse(szBuf);//spec_dataΪ��ȡ����JSON�ļ�
        if(spec_data)
        {
            cJSON *id = cJSON_GetObjectItem(spec_data,"site_id");
            if(id)Terminal.SITE_ID = (__u64)id->valuedouble;

            cJSON *password = cJSON_GetObjectItem(spec_data,"site_password");
            if(password)strcpy(Terminal.SALT_password,password->valuestring);

            cJSON *salt = cJSON_GetObjectItem(spec_data,"site_salt");
            if(salt)strcpy(Terminal.SALT_site,salt->valuestring);

            cJSON *server = cJSON_GetObjectItem(spec_data,"server_dns");
            if(server)strcpy(Terminal.Server,server->valuestring);

            cJSON *server_ip = cJSON_GetObjectItem(spec_data,"server_ip");
            if(server_ip)strcpy(Terminal.Server_IP,server_ip->valuestring);

            cJSON *server_port = cJSON_GetObjectItem(spec_data,"server_port");
            if(server_port)Terminal.Server_Port = server_port->valueint;

            cJSON *wifi_pass = cJSON_GetObjectItem(spec_data,"wifi_password");
            if(wifi_pass)strcpy(Terminal.Wifi_password,wifi_pass->valuestring);

            cJSON *wifi_name = cJSON_GetObjectItem(spec_data,"wifi_name");
            if(wifi_name)strcpy(Terminal.Wifi_name,wifi_name->valuestring);

            cJSON *backstage_sel = cJSON_GetObjectItem(spec_data,"backstage_sel");
            if(backstage_sel)Terminal.BackStage_Sel = backstage_sel->valueint;

            cJSON_Delete(spec_data);
        }
    }
    else perror("open /site_info");

    memset(szBuf,0,sizeof(szBuf));
    if((fd = open("/connect_record",O_CREAT | O_RDWR,0600)) > 0)
    {
        while(flock(fd,LOCK_EX|LOCK_NB));
        lseek(fd,0,SEEK_SET);
        if(read(fd,szBuf,sizeof(szBuf)) == -1)perror("read");
        while(flock(fd,LOCK_UN));
        close(fd);

        spec_data = cJSON_Parse(szBuf);//spec_dataΪ��ȡ����JSON�ļ�
        if(spec_data)
        {
            cJSON *ConnectCountPtr = cJSON_GetObjectItem(spec_data,"ConnectCount");
            if(ConnectCountPtr)Terminal.ConnectCount = ConnectCountPtr->valuedouble;

            cJSON *DisConnectCountPtr = cJSON_GetObjectItem(spec_data,"DisConnectCount");
            if(DisConnectCountPtr)Terminal.DisConnectCount = DisConnectCountPtr->valuedouble;

            cJSON *ResetCountPtr = cJSON_GetObjectItem(spec_data,"ResetCount");
            if(ResetCountPtr)Terminal.ResetCount = ResetCountPtr->valuedouble;

            cJSON_Delete(spec_data);
        }
    }
    else perror("open /connect_record");

    memset(szBuf,0,sizeof(szBuf));
    if((fd = open("/version",O_CREAT | O_RDWR,0600)) > 0)
    {
        while(flock(fd,LOCK_EX|LOCK_NB));
        lseek(fd,0,SEEK_SET);
        if(read(fd,szBuf,sizeof(szBuf)) == -1)perror("read");
        while(flock(fd,LOCK_UN));
        close(fd);

        spec_data = cJSON_Parse(szBuf);//spec_dataΪ��ȡ����JSON�ļ�
        if(spec_data)
        {
            cJSON *soft_version = cJSON_GetObjectItem(spec_data,"soft_version");
            if(soft_version)strcpy(Terminal.Software_version, soft_version->valuestring);

            cJSON *hard_version = cJSON_GetObjectItem(spec_data,"hard_version");
            if(hard_version)strcpy(Terminal.Hardware_version, hard_version->valuestring);

            cJSON_Delete(spec_data);
        }
    }
    else perror("open /version");

    printf("��ɶ�ȡ�����ļ�\r\n");
}

//��APPFLAG��־λ
int ReadFlag()
{
    int result = 0;
    char *path = "/version";
    char szBuf[128] = {0};
    int fd;
    if((fd = open(path,O_CREAT | O_RDWR,0600)) < 0)perror("open");
    while(flock(fd,LOCK_EX|LOCK_NB));
    lseek(fd,0,SEEK_SET);
    if(read(fd,szBuf,sizeof(szBuf)) == -1)perror("read");
    while(flock(fd,LOCK_UN));
    close(fd);

    cJSON *spec_data = cJSON_Parse(szBuf);
    if(spec_data)
    {
        cJSON *appflag = cJSON_GetObjectItem(spec_data,"appflag");
        if(appflag && APP_FLAG == (__u64)appflag->valuedouble)result = 1;
        cJSON_Delete(spec_data);
    }
    return result;
}
__u8 SendFlag = 1;
/**************************************
�������ƣ�HeartAlarmHandler()
������������ʱ����������
���룺
�����
������Ϣ:
***************************************/
void HeartAlarmHandler(int i)
{
    if(SITE_state == LOAD_ON && SendFlag)
    {
        struct Terminal_MsgSt NetSend_queue = {0};
        NetSend_queue.MsgType = IPC_NOWAIT;
        NetSend_queue.MsgBuff[1] = ERROR_APP;
        msgsnd(CanRecv_qid, &NetSend_queue, 2, IPC_NOWAIT);
    }
    alarm(3);
}

void initTerminal()
{
    Terminal.SITE_ID = 9999999999999ll; //ֻ�ڿ�ʼ��ʱ�򸳳�ֵ
    strcpy(Terminal.SALT_password,"123456");
    strcpy(Terminal.SALT_site,"lurZQ0");
    strcpy(Terminal.Server,"ebikeconsole.idmakers.cn");
    strcpy(Terminal.Hardware_version,"20170921");
    strcpy(Terminal.Software_version,"20170921");
}

void prepare()
{
    signal(SIGPIPE,SIG_IGN);//���޶����̵Ĺܵ�д����
    signal(SIGALRM,HeartAlarmHandler);

    ResetRecord();          //��¼��������
    File_Configuration();   //��ȡ�����ļ�
    init_mutex();           //��ʼ��������
//	set_GPIOG_0_low();//�澯
//	set_GPIOG_1_low();//ͨѶ
    set_GPIOG_2_low();//����
    set_GPIOG_3_low();//����
    /*************Creat Queue************/
    while(CanRecv_qid<=0)
    {
        CanRecv_qid = msgget(IPC_PRIVATE,IPC_CREAT|0666);
        if(CanRecv_qid<=0)printf("creat CanRecv queue failed !\r\n");

        struct timeval timeout={0,10000};
        select(0,NULL,NULL,NULL,&timeout);
    }
    printf("CanRecv_qid is : %d \r\n",CanRecv_qid);
    while(UpdataRecv_qid<=0)
    {
        UpdataRecv_qid = msgget(IPC_PRIVATE,IPC_CREAT|0666);
        if(UpdataRecv_qid<=0)printf("creat UpdataRecv queue failed !\r\n");

        struct timeval timeout={0,10000};
        select(0,NULL,NULL,NULL,&timeout);
    }
    printf("UpdataRecv_qid is : %d \r\n",UpdataRecv_qid);
    printf("Bootloader pid is:%d\r\n",getpid());

    if(Terminal.BackStage_Sel == 1)
        tcp_func = TCP_SW;
    else
        tcp_func = TCP_KPD;
}

void main_loop()
{
    pthread_t LED_tid,CAN_tid,NET_tid;
    if(pthread_create(&LED_tid,NULL,LED_thread,NULL))  	//����LED�߳�
    {
        perror("Create LED thread error!");
        return;
    }
    if(pthread_create(&CAN_tid,NULL,CAN_thread,NULL))   //����CAN�߳�
    {
        perror("Create CAN thread error!");
        return;
    }
    if(pthread_create(&NET_tid,NULL,NET_thread,NULL))   //����NET�߳�
    {
        perror("Create NET thread error!");
        return;
    }
    /**************************/
    //�ȴ��߳�
    /**************************/
    if(pthread_join(CAN_tid,NULL))			//�ȴ�CAN�߳�
    {
        perror("wait CAN thread error!!");
        return;
    }
    if(pthread_join(NET_tid,NULL))			//�ȴ�NET�߳�
    {
        perror("wait NET thread error!!");
        return;
    }
    if(pthread_join(LED_tid,NULL))			//�ȴ�LED�߳�
    {
        perror("wait LED thread error!!");
        return;
    }
    close(CAN0_fd);
    close(USART_fd);

    if(msgctl(CanRecv_qid,IPC_RMID,0)<0) perror("ɾ��CanRecv��Ϣ���д���");
    if(msgctl(NetRecv_qid,IPC_RMID,0)<0) perror("ɾ��NetRecv��Ϣ���д���");
}

int main(int argc,char *argv[])
{
    printf("/**************************************����BOOTLOADER**************************************/\r\n");
    isRunning = state = PROGRAM_BOOTLOADER;
    prepare();//׼������

    if(argc > 1 && PROGRAM_VIRTUAL_SERVER == argv[1][0])//��������ҵ�һ��������PROGRAM_VIRTUAL_SERVER��Ϊ�������������ת����
    {
        ATState = argv[3][0];
        SendFlag = 0;
        if(ATState == NOT_TO_INIT)
        {
            SITE_state = LOAD_ON;//AT�����ʼ��ʱ����Ҫ��½
            strcpy(Terminal.TOKEN_site,argv[4]);
            printf("���������������Bootloader\r\nFrom:%s To:%s ATInit:%s Site_comm_token:%s\r\n",argv[1],argv[2],argv[3],argv[4]);
            struct Terminal_MsgSt NetRecv_queue = {0};
            NetRecv_queue.MsgType = IPC_NOWAIT;
            NetRecv_queue.MsgBuff[1] = BOOTLOAD_IN;
            msgsnd(CanRecv_qid, &NetRecv_queue, 2, IPC_NOWAIT);//��ʱҪ���͸���λ���ķ���:�ѽ���Bootloader
        }
        main_loop();//�������ѭ��
    }
    else if(!ReadFlag())//����һ����Ϊ���ϵ�ֱ����ת ����Ҫ�ж�APPFlag
    {
        printf("��ȡAppFlagʧ�� ��Bootloader��ֱ���������������������\r\n");
        main_loop();//��δ��λ �������ѭ��
    }
    else printf("��ȡAppFlag�ɹ� ��ʼ����Ӧ�ó������������������......\r\n");

    printf("׼������APP���������������...\r\n");
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

    //������һ������
    char cmd[50] = {0};
    printf("\r\nt2 begin to work...,and use kill -9 %d to terminate\r\n",getpid());
    sprintf(cmd,"%s %c %c %c %s","./t2",PROGRAM_BOOTLOADER,PROGRAM_VIRTUAL_SERVER,ATState,Terminal.TOKEN_site);
    printf("cmd is:%s\r\n",cmd);
    pid_t status = system(cmd);
    if (-1 == status)
    {
        printf("system error!");
    }
    else
    {
        printf("exit status value = [0x%x]\n", status);
        if (WIFEXITED(status))
        {
            if (0 == WEXITSTATUS(status))
            {
                printf("run shell script successfully.\n");
            }
            else
            {
                printf("run shell script fail, script exit code: %d\n", WEXITSTATUS(status));
            }
        }
        else
        {
            printf("exit status = [%d]\n", WEXITSTATUS(status));
        }
    }
}
