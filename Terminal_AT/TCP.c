#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <linux/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <math.h>
#include <semaphore.h>
#include "Terminal.h"
#include "UDP.h"
#include "CommonFunc.h"

static int total_knee,total_bike;
static int check_flag = 0;
static int not_knee_check = 0;
static double knees_mode = 1;
static int force_unlock[EQUIP_NUM_MAX+1] = {0};

extern __u8 SITE_state;
extern struct Terminal_Info Terminal;
extern struct Knee_struct Knee_can[EQUIP_NUM_MAX+1];
extern struct Net_struct Net_data[EQUIP_NUM_MAX+1];
extern struct CheckKnee_Struct CheckKnee[EQUIP_NUM_MAX+1];
extern struct UdpScan_struct   UpScan_Scan[EQUIP_NUM_MAX + 1];
extern __u64 Modify_knee_id[EQUIP_NUM_MAX+1];

extern int isDisconnected;
extern int lowBattery;
extern int Server_time;
extern int Green_light;
extern int Red_light;
extern RUNNING_STATE isRunning;

static int NetSendData(int sockfd)
{
    if(SITE_state == NO_LOAD)return 0;
    __u16 functioncode;

    struct Terminal_MsgSt NetSend_queue = {0};
    NetSend_queue.MsgType = IPC_NOWAIT;

    int temp = msgrcv(CanRecv_qid, &NetSend_queue, 2, 0, IPC_NOWAIT);
    if(temp >= 0)
    {
        __u32 function_code = NetSend_queue.MsgBuff[1];
        __u16 equip_num = NetSend_queue.MsgBuff[0];
        if(equip_num < 0 || equip_num > EQUIP_NUM_MAX)
        {
            return -1;
        }
        cJSON *data_json = cJSON_CreateObject();
        pthread_mutex_lock(&Knee_can[equip_num].mt);
        switch(function_code)
        {
            case CUSTOMER_ID_CHECK:
            {
                printf("�޸Ŀͻ���ŷ���\r\n");
                functioncode = GET_CUSTOMER_ID_BACK;
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "return_result", 1);
                cJSON_AddNumberToObject(data_json, "site_id", Terminal.SITE_ID);
                break;
            }
            default:
            {
                break;
            }
        }
        pthread_mutex_unlock(&Knee_can[equip_num].mt);
        /********************************/
        char *spec_data = data_json->child?cJSON_Print(data_json):NULL;
        cJSON_Delete(data_json);
        if(spec_data)
        {
            char data_send[1024] = {0};
            printf("spec_data is:%s\r\n",spec_data);
            Protocol_make(spec_data,data_send,1,PACKAGE_state,functioncode);
            if(write(sockfd,data_send,strlen(spec_data)+43) < 0)perror("Send");
            free(spec_data);

            struct timeval timeout={0,100000};
            select(0,NULL,NULL,NULL,&timeout);
        }
        else printf("cJSON malloc error");
        /********************************/
    }
}

static int RecvNetData(int sockfd)
{
    if(SITE_state == NO_LOAD)return 0;
    __u32 t;
    int recv_len;
    __u64 knee_SN;
    __u8 knee_NUM = 66;
    char data_recv[1024] = {0};

    struct Terminal_MsgSt NetRecv_queue = {0};
    NetRecv_queue.MsgType = IPC_NOWAIT;

    struct timeval timeout={0,1000};
    fd_set rd;
    FD_ZERO(&rd);
    FD_SET(sockfd,&rd);
    if(FD_ISSET(sockfd,&rd) && select(sockfd+1,&rd,NULL,NULL,&timeout)>0)
    {
        while((recv_len = read(sockfd,data_recv,1)) > 0)
        {
            if(data_recv[0] != 0x40)
            {
                continue;
            }
            read(sockfd, data_recv + 1, 1);
            if(data_recv[1] != 0x3A)
            {
                continue;
            }
            read(sockfd, data_recv + 2, 1);
            read(sockfd, data_recv + 3, 1);
            __u32 json_len = (data_recv[2]<<8) + data_recv[3] - 35;
            recv_len = json_len + 35 + 4;
            while((recv_len -= read(sockfd, data_recv + 4 + (json_len + 35 + 4 - recv_len), recv_len))>0)
            {
                struct timeval timeout={0,1000};
                select(0,NULL,NULL,NULL,&timeout);
            }
            if(data_recv[41+json_len]!=0x0D || data_recv[42+json_len]!=0x0A)
            {
                printf("Tail Error!!\r\n");
                break;
            }
            __u16 crc = CRC16_CCITT_XMODEM(data_recv + 4,json_len + 35);
            if((crc>>8)!=data_recv[39+json_len] || (crc&0xFF)!=data_recv[40+json_len])
            {
                printf("CRC error!!\r\n");
                break;
            }
            char data_recv_json[1024] = {0};
            for(t=0;t<json_len;t++)
            {
                data_recv_json[t] = data_recv[39+t];
            }
            printf("data_recv_json is : %s\r\n",data_recv_json);

            __u64 functioncode = (data_recv[37]<<8) + data_recv[38];

            cJSON *spec_data = cJSON_Parse(data_recv_json);
            switch(functioncode)
            {
                case SCAN_VERSION:
                {
                    printf("ɨ��");
                    cJSON *scan_dev_p = cJSON_GetObjectItem(spec_data,"update_dev");
                    if(scan_dev_p)
                    {
                        switch(scan_dev_p->valueint)
                        {
                            case 1:
                            {
                                printf("�ն�\r\n");
                                NetRecv_queue.MsgBuff[1] = EQUIP_ACCEPT_BACK;
                                msgsnd(CanRecv_qid, &NetRecv_queue, 2, IPC_NOWAIT);
                                break;
                            }
                            case 2:
                            {
                                printf("����\r\n");
                                NetRecv_queue.MsgBuff[1] = EQUIP_MCU;
                                break;
                            }
                            case 3:
                            {
                                printf("�Կ�\r\n");
                                NetRecv_queue.MsgBuff[1] = EQUIP_DIS;
                                break;
                            }
                            case 4:
                            {
                                printf("�ز�\r\n");
                                NetRecv_queue.MsgBuff[1] = EQUIP_CARRIER;
                                break;
                            }
                            case 5:
                            {
                                printf("�ֿ�LOGO\r\n");
                                NetRecv_queue.MsgBuff[1] = EQUIP_LOGO;
                                break;
                            }
                            case 6:
                            {
                                printf("�ϵ縴λ��\r\n");
                                NetRecv_queue.MsgBuff[1] = EQUIP_PCRD;
                                break;
                            }
                            default:printf("�Ƿ�����\r\n");break;
                        }
                    }
                    break;
                }
                default:
                {
                    break;
                }
            }
            cJSON_Delete(spec_data);
            if(msgsnd(NetRecv_qid, &NetRecv_queue, 2, IPC_NOWAIT) < 0)perror("NET msgsnd");
            break;
        }
    }
}

void TCP_loop(int sockfd)
{
    NetSendData(sockfd);
    RecvNetData(sockfd);
}

int TCP_init(void)
{
    int TCP_fd = 0;
    struct sockaddr_in my_addr;
    struct timeval timeout= {10,0};
    /*build and initialize the SOCKET*/
    TCP_fd = socket(AF_INET,SOCK_STREAM,0);
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(TCP_PORT);
    my_addr.sin_addr.s_addr = inet_addr(str);
    memset(&my_addr.sin_zero,0,8);
    setsockopt(TCP_fd,SOL_SOCKET,SO_SNDTIMEO,(char*)&timeout,sizeof(struct timeval));
    printf("listening......\r\n");
    listen(1);
    int tmp_int = accept(TCP_fd,(struct sockaddr*)&my_addr,sizeof(struct sockaddr));
    if(tmp_int < 0)
    {
        printf("ERROR:connect erro %s!\n",strerror(errno));
        close(TCP_fd);
        return -1;
    }
    printf("connect OK.\r\n");
    return TCP_fd;
}
