#include <arpa/inet.h>
#include <sys/socket.h>
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
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/msg.h>
#include <fcntl.h>
#include "UDP.h"
#include "TCP.h"
#include "can.h"
#include "Terminal.h"
#include "CJSON.h"

#define BUF_LEN 128
#define SERVER_PORT 8080

struct sockaddr_in addr,PC_addr;
socklen_t PC_addrlen = sizeof(struct sockaddr_in);

int UDP0_fd = 0;            //ETH0(HTTP)
int Boot_load = 2;          //是否进入Bootload状态：1已进入 2已退出	
__u32 code_length = 0;      //需要写入的代码长度
static __u32 packet_total = 0;      //总共包含packet_total个数据包
__u16 packet_n = 1;          //当前包序号
__u16 packet_n_recv = 1;     //接收到数据的包序号
static __u8 start_addr[4] = {0};       //程序烧写总的起始地址

extern int CAN0_fd;      //CAN0
extern struct UpData_struct    UpData_data;
extern struct UdpScan_struct   UpScan_Scan[EQUIP_NUM_MAX + 1];
extern int Net_Pause;

extern __u64 SITE_ID;                           //站点ID
extern char SALT_site[10];                      //站点盐值
extern char SALT_password[32];                  //站点密码
extern char Server[32];                         //站点域名
extern char Server_IP[32];                      //服务器IP
extern char Wifi_password[32];                  //Wifi密码
extern char Wifi_name[32];                      //Wifi名
extern int Server_Port;                         //服务器PORT

int Resend = 0;         //超时重发计时器
int IsResend = 0;       //超时重发计时标志
char ResendText[23];    //超时重发内容

/**************************************
函数名称：GetLocalIP
功能描述：获取本地eth0的IP
输入：
输出：
创建信息：
***************************************/
char* GetLocalIP(void)
{
    int inet_sock;
    struct ifreq ifr;
    char *ip = (char *)malloc(32);
    inet_sock = socket(AF_INET,SOCK_DGRAM,0);
    strcpy(ifr.ifr_name , "eth0");
    ioctl(inet_sock,SIOCGIFADDR,&ifr);
    strcpy(ip,inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    return ip;
}
/**************************************
函数名称：UDP0_init
功能描述：初始化UDP套接字
输入：
输出：
创建信息：
***************************************/
int UDP0_init(void)
{
    int sockfd;
    int opt = 1;
    char *IP=NULL;

    IP = GetLocalIP();
    printf("IP:%s\r\n",IP);
    sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd < 0)
    {
        perror("socket creation failed!\n");
        return -1;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = inet_addr(IP);
    free(IP);
    memset(&(addr.sin_zero),0,8);
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    if(bind(sockfd,(struct sockaddr *)&addr,sizeof(struct sockaddr_in)))
    {
        perror("socket binding failed!\n");
        return -1;
    }
    return sockfd;
}
/**************************************
函数名称：UpdataSend
功能描述：主线程中接收Can的消息队列指令并发送至上位机
输入：
输出：
创建信息：
***************************************/
int UpdataSend(int sockfd)
{
    struct Terminal_MsgSt UpdataSend_queue = {0};
    UpdataSend_queue.MsgType = IPC_NOWAIT;

    int temp = msgrcv(UpdataSend_qid, &UpdataSend_queue, 2, 0, IPC_NOWAIT);
    if(temp >= 0)
    {
        switch(UpdataSend_queue.MsgBuff[1])
        {
            case EQUIP_ACCEPT_BACK:
            {
                printf("桩体扫描返回\r\n");
                Equip_accept_back((char*)&(UpScan_Scan[UpdataSend_queue.MsgBuff[0]]),UpdataSend_queue.MsgBuff[0]);
                memset(&UpScan_Scan[UpdataSend_queue.MsgBuff[0]],0,sizeof(struct UdpScan_struct));
                break;
            }
            case ERRASE_BACK:
            {
                printf("NET 擦除返回\r\n");
                if(UpData_data.state == 0x02)    //擦除成功
                {
                    UpData_data.package_count = 1;
                    Program_errase_back(UpData_data.data ,0x01);
                }
                else                             //擦除失败
                {
                    Program_errase_back(UpData_data.data ,0x00);
                }
                break;
            }
            case PROGRAM_WRITE_BACK:
            {
//              printf("NET 写入程序包返回\r\n");
                Program_send_back(UpData_data.data , UpData_data.program_state);
                break;
            }
            case PROGRAM_WRITE_FINISH:
            {
                printf("NET 程序烧写完成\r\n");
                IsResend = 0;                   //烧写完成时重新复位超时重发计时标志
                Program_finish_back(UpData_data.data , UpData_data.program_state);
                break;
            }
            default:
            {
                break;
            }
        }
    }
}
/**************************************
函数名称：RecvUpdata
功能描述：主线程中接收上位机指令并推入Can的消息队列
输入：
输出：
创建信息：
***************************************/
int RecvUpdata(int sockfd)
{
    int temp = 0;
    int udp_len = 0;
    __u32 order_len = 0;
    char order_check = 0;
    __u8 knee_num = 255;             //桩体编号

    char recv_order[512] = {0};
    int frame_num = 0;          //UDP接收包序号
    int equip_name = 0;         //设备名称
    int equip_type = 0;         //设备型号
    int order_num = 0;          //命令号

    struct timeval timeout={0,10000};
    setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));   // 设置接收等待超时
    udp_len = recvfrom(sockfd,recv_order,512,0,(struct sockaddr *)&PC_addr,&PC_addrlen);

    //超时重发判断
    if(IsResend && Resend++ == 200)
    {
        Resend = 0;
        IsResend = 0;
        int i = sendto(UDP0_fd,ResendText,23,0,(struct sockaddr *)&PC_addr,PC_addrlen);
        if(i > 0){
            printf("超时重发成功\r\n");
        }
        else{
            printf("超时重发失败\r\n");
        }
    }

    //判断帧头、帧尾
    if((*(recv_order+0) == 0x74)&&(*(recv_order+1) == 0x6F)&&(*(recv_order+2) == 0x65)&&(*(recv_order+3) == 0x63)&&(*(recv_order+udp_len-1) == 0x64)&&(*(recv_order+udp_len-2) == 0x6e)&&(*(recv_order+udp_len-3) == 0x65)&&(*(recv_order+udp_len-4) == 0x3a))
    {
//      printf("帧头、帧尾正确\r\n");
        //判断校验位
        order_len = 13+*(recv_order+9)*16777216+*(recv_order+10)*65536+*(recv_order+11)*256+*(recv_order+12)*1;
        for(temp = 0 ; temp < order_len ; temp++)
        {
            order_check ^=  *(recv_order+temp);
        }
        if(order_check == *(recv_order+order_len))
        {
//          printf("校验正确\r\n");
            frame_num = *(recv_order+4);
//          pro_version = *(recv_order+5);        //协议版本
            equip_type = *(recv_order+7);         //设备型号
            order_num = *(recv_order+8);          //命令号
            knee_num = *(recv_order+13);          //桩体编号
            equip_name = *(recv_order+14);        //程序类型    0x01主控 0x02显控 0x03数据终端 0x04载波控制 0x05单铁桩

            struct Terminal_MsgSt UpdataRecv_queue = {0};
            UpdataRecv_queue.MsgType = IPC_NOWAIT;
            switch(order_num)
            {
                case SERVER_PORT_WRITE:
                {
                    printf("重新写入服务器PORT\r\n");
                    for(temp =0;temp<udp_len;temp++)
                    {
                        UpData_data.data[temp] = *(recv_order+temp);
                    }

                    __u32 len = ((*(recv_order+11)<<8)&0xFF00) + *(recv_order+12) - 9;
                    Server_Port = 58000;
                    site_id_reset(&Server_Port,SERVER_PORT_WRITE);

                    Configure_back(UpData_data.data,1);
                    system("reboot");
                    break;
                }
                case SERVER_IP_WRITE:
                {
                    printf("重新写入服务器IP\r\n");
                    for(temp =0;temp<udp_len;temp++)
                    {
                        UpData_data.data[temp] = *(recv_order+temp);
                    }

                    __u32 len = ((*(recv_order+11)<<8)&0xFF00) + *(recv_order+12) - 9;
                    strncpy(Server_IP,recv_order + 13,len);
                    site_id_reset(Server_IP,SERVER_IP_WRITE);

                    Configure_back(UpData_data.data,1);
                    system("reboot");
                    break;
                }
                case SINGLE_NET:
                {
                    for(temp =0;temp<udp_len;temp++)
                    {
                        UpData_data.data[temp] = *(recv_order+temp);
                    }
                    if((*(recv_order+13)==0xFF)&&(*(recv_order+14)==0xFF))
                    {
                        printf("单机版\r\n");
                        Net_Pause = 1;
                        UpdataRecv_queue.MsgBuff[1] = SINGLE;
                    }
                    else if ((*(recv_order+13)==0xFF)&&(*(recv_order+14)==0xFE))
                    {
                        printf("网络版\r\n");
                        Net_Pause = 0;
                        UpdataRecv_queue.MsgBuff[1] = NET;
                    }
                    Configure_back(UpData_data.data,1);
                    break;
                }
                case IMMEDIATE_UNLOCK:
                {
                    printf("应急开锁\r\n");
                    UpdataRecv_queue.MsgBuff[1] = IMMEDIATE_UNLOCK;
                    UpdataRecv_queue.MsgBuff[0] = knee_num;            //桩位号
                    break;
                }
                case SITE_ID_WRITE:
                {
                    printf("重新写入站点号\r\n");
                    for(temp =0;temp<udp_len;temp++)
                    {
                        UpData_data.data[temp] = *(recv_order+temp);
                    }
                    SITE_ID = Hex_Dec(recv_order+13,order_len - 13);
                    site_id_reset(&SITE_ID,SITE_ID_WRITE);

                    Configure_back(UpData_data.data,1);
                    system("reboot");
                    break;
                }
                case SITE_PASSWORD_WRITE:
                {
                    printf("重新写入站点密码\r\n");
                    for(temp =0;temp<udp_len;temp++)
                    {
                        UpData_data.data[temp] = *(recv_order+temp);
                    }
                    memset(SALT_password,0,sizeof(SALT_password));
                    strncpy(SALT_password,recv_order + 13,order_len - 13);
                    site_id_reset(SALT_password,SITE_PASSWORD_WRITE);

                    Configure_back(UpData_data.data,1);
                    system("reboot");
                    break;
                }
                case SITE_DNS_WRITE:
                {
                    printf("重新写入站点域名\r\n");
                    for(temp =0;temp<udp_len;temp++)
                    {
                        UpData_data.data[temp] = *(recv_order+temp);
                    }
                    memset(Server,0,sizeof(Server));
                    strncpy(Server,recv_order + 13,order_len - 13);
                    site_id_reset(Server,SITE_DNS_WRITE);

                    Configure_back(UpData_data.data,1);
                    system("reboot");
                    break;
                }
                case EQUIP_MAINTAIN_ENTER:
                {
                    if((*(recv_order+13)==0xFF)&&(*(recv_order+14)==0xFF))
                    {
                        printf("开始升级,进入维护状态,断开后台连接\r\n");
                        Net_Pause = 1;
                        UpdataRecv_queue.MsgBuff[1] = EQUIP_MAINTAIN_ENTER;
                    }
                    else if ((*(recv_order+13)==0xFF)&&(*(recv_order+14)==0xFE))
                    {
                        printf("停止升级,退出维护状态,恢复后台连接\r\n");
                        Net_Pause = 0;
                        UpdataRecv_queue.MsgBuff[1] = EQUIP_MAINTAIN_EXIT;
                    }
                    break;
                }
                case EQUIP_ACCEPT:
                {
                        printf("设备扫描\r\n");
                        UpdataRecv_queue.MsgBuff[1] = EQUIP_ACCEPT;
                        break;
                }
                case ERRASE:
                {
                    printf("程序擦除\r\n");
                    for(temp =0;temp<udp_len;temp++)
                    {
                        UpData_data.data[temp] = *(recv_order+temp);
                    }
                    UpdataRecv_queue.MsgBuff[0] = knee_num;            //桩位号
                    UpData_data.equip_name = equip_name;               //程序类型
                    for(temp=0;temp<4;temp++)
                    {
                        UpData_data.errase_addr[temp] = *(recv_order+15+temp);   //擦除起始地址
                        UpData_data.errase_size[temp] = *(recv_order+19+temp);   //擦除区域大小
                    }
                    UpData_data.state = 0x00;   				//处于应用程序中
                    UpdataRecv_queue.MsgBuff[1] = ERRASE;
                    break;
                }
                case PROGRAM_WRITE:
                {
//                  printf("程序写入\r\n");
                    IsResend = 0;                                     //接收到了上位机回复清除重发标志
                    Resend = 0;                                       //接收到了上位机回复清除重发计时

                    __u32 code_length = 0;
                    UpData_data.package_num = ((*(recv_order+16)<<8)&0xFF00) + *(recv_order+17);     //包序号
                    //数据终端自身升级第一包 package_count自增到1 删除t2
                    if(equip_name == TERMINAL && UpData_data.package_num == 1){
                        if(remove("/update/t2")==0)
                        {
                            printf("t2删除成功\n");
                        }
                        UpData_data.package_count = 1;
                    }
                    if(UpData_data.package_num == UpData_data.package_count)    //包序号与包计数相同
                    {
                        UpData_data.equip_name = equip_name;            //程序类型

                        for(temp =0;temp<udp_len;temp++)
                        {
                            UpData_data.data[temp] = *(recv_order+temp);
                        }
                        for(temp=0;temp<4;temp++)
                        {
                            UpData_data.program_size[temp] = *(recv_order+18+temp);   //程序大小
                            UpData_data.program_addr[temp] = *(recv_order+22+temp);   //写入起始地址
                        }
                        code_length = ((*(recv_order+18)<<24)&0xFF000000) + ((*(recv_order+19)<<16)&0x00FF0000) + ((*(recv_order+20)<<8)&0x0000FF00) + *(recv_order+21);    //计算需要写入的代码长度
                        if(code_length%256 == 0)        //计算数据包的个数
                        {
                            UpData_data.packet_total = code_length/256;
                        }
                        else
                        {
                            UpData_data.packet_total = code_length/256 + 1;
                        }

                        if(equip_name == TERMINAL) //升级数据终端
                        {
                            if(Terminal_Updata((recv_order+26),256,""))   //写入成功
                            {
                                if(UpData_data.package_count == UpData_data.packet_total)      //程序烧写完成
                                {
                                    UpdataRecv_queue.MsgBuff[1] = PROGRAM_WRITE_FINISH;
                                    msgsnd(UpdataSend_qid, &UpdataRecv_queue, 2, IPC_NOWAIT);
                                    system("chmod 777 /update/t2");
                                    system("mv -f /update/t2 /t2");
                                    system("reboot");
                                }
                                else
                                {
                                    UpData_data.package_count++;        //计数加1，等待写入下一包
                                    UpData_data.program_state = 0x01;
                                    printf("包写入成功\r\n");
                                }
                            }
                            else
                            {
                                UpData_data.program_state = 0x00;
                                if(remove("/update/t2")==0){printf("写入失败，删除t2\r\n");}
                                printf("包写入失败\r\n");
                            }
                            UpdataRecv_queue.MsgBuff[1] = PROGRAM_WRITE_BACK;
                            msgsnd(UpdataSend_qid, &UpdataRecv_queue, 2, IPC_NOWAIT);
                            return 0;
                        }
                        else                        //升级其他设备
                        {
                            UpdataRecv_queue.MsgBuff[0] = knee_num;         //桩子编号
                            UpdataRecv_queue.MsgBuff[1] = PROGRAM_WRITE;
                        }

                    }
                    else
                    {
                        //程序写入失败，重新写,告诉上位机失败重发上一包
                        printf("包序号与计数不同 终端计数:%d 上位机计数:%d\r\n",UpData_data.package_count,UpData_data.package_num);
                        UpData_data.program_state = 0x00;
                        printf("包写入失败\r\n");
                        UpdataRecv_queue.MsgBuff[1] = PROGRAM_WRITE_BACK;
                        msgsnd(UpdataSend_qid, &UpdataRecv_queue, 2, IPC_NOWAIT);
                        return 0;
                    }
                    break;
                }
                case BOOT_IN:
                {
                    break;
                }
                case BOOT_OUT:
                {
                    break;
                }
                default:
                {
                    break;
                }
            }
            temp = msgsnd(UpdataRecv_qid, &UpdataRecv_queue, 2, IPC_NOWAIT);         //压入UpdataRecv_queue队列
//          printf("UDP temp is : %d\r\n",temp);
        }
    }
}
/**************************************
函数名称：Program_errase_back
功能描述：程序扫描结果发送到上位机
输入：
输出：
创建信息：
***************************************/
int Equip_accept_back(char *order,char num)
{
        int t = 0;
        char send_check = 0;
        char *send_order = NULL;
        struct UdpScan_struct *p = (struct UdpScan_struct*)order;

        send_order = (char *)malloc(35);
        memset(send_order,0,35);
        *(send_order+0) = 0x74;
        *(send_order+1) = 0x6F;
        *(send_order+2) = 0x65;
        *(send_order+3) = 0x63;

        *(send_order+8) = 0x01;
        *(send_order+9) = 0x00;
        *(send_order+10) = 0x00;
        *(send_order+11) = 0x00;
        *(send_order+12) = 17;
        *(send_order+13) = num;

        *(send_order+17) = (p->MH>>24) & 0xff;
        *(send_order+16) = (p->MH>>16) & 0xff;
        *(send_order+15) = (p->MH>>8) & 0xff;
        *(send_order+14) = p->MH & 0xff;

        *(send_order+21) = (p->MS>>24) & 0xff;
        *(send_order+20) = (p->MS>>16) & 0xff;
        *(send_order+19) = (p->MS>>8) & 0xff;
        *(send_order+18) = p->MS & 0xff;

        *(send_order+25) = (p->DH>>24) & 0xff;
        *(send_order+24) = (p->DH>>16) & 0xff;
        *(send_order+23) = (p->DH>>8) & 0xff;
        *(send_order+22) = p->DH & 0xff;

        *(send_order+29) = (p->DS>>24) & 0xff;
        *(send_order+28) = (p->DS>>16) & 0xff;
        *(send_order+27) = (p->DS>>8) & 0xff;
        *(send_order+26) = p->DS & 0xff;

        for(t = 0 ; t <= 29 ; t++)
        {
            send_check ^=  *(send_order+t);
        }
        *(send_order+30) = send_check;  //异或校验位
        send_check = 0;
        *(send_order+31) = 0x3A;    //':'
        *(send_order+32) = 0x65;    //'e'
        *(send_order+33) = 0x6E;    //'n'
        *(send_order+34) = 0x64;    //'d'
        int i = sendto(UDP0_fd,send_order,35,0,(struct sockaddr *)&PC_addr,PC_addrlen);
        if(i<=0){
            printf("Errno:%d %s\r\n",errno,strerror(errno));
        }
        printf("Send Success\r\n");
        free(send_order);
        return 0;
}
/**************************************
函数名称：Program_errase_back
功能描述：程序擦除成功发送到上位机
输入：
输出：
创建信息：
***************************************/
int Program_errase_back(char *order , int send_state)
{
    int t = 0;
    char send_check = 0;
    char *send_order = NULL;

    send_order = (char *)malloc(21);
    memset(send_order,0,21);
    for(t = 0; t <= 8; t++)
    {
        *(send_order+t) = *(order+t);
    }
    *(send_order+9) = 0x00;
    *(send_order+10) = 0x00;
    *(send_order+11) = 0x00;
    *(send_order+12) = 0x03;
    *(send_order+13) = *(order+13);
    *(send_order+14) = *(order+14);                 //充电桩或显控程序
    if(send_state == 1)
    {
        *(send_order+15) = 0x01;         //发送成功
    }
    else
    {
        *(send_order+15) = 0x00;         //发送失败
    }
    for(t = 0 ; t <= 15 ; t++)
    {
        send_check ^=  *(send_order+t);
    }
    *(send_order+16) = send_check;  //异或校验位
    send_check = 0;
    *(send_order+17) = 0x3A;    //':'
    *(send_order+18) = 0x65;    //'e'
    *(send_order+19) = 0x6E;    //'n'
    *(send_order+20) = 0x64;    //'d'
    int i = sendto(UDP0_fd,send_order,21,0,(struct sockaddr *)&PC_addr,PC_addrlen);
    if(i<=0){
        printf("Errno:%d %s\r\n",errno,strerror(errno));
    }
    free(send_order);
    return 0;
}
/**************************************
函数名称：Program_send_back
功能描述：程序升级每包成功发送到上位机
输入：
输出：
创建信息：
***************************************/
int Program_send_back(char *order, int send_state)
{
    int t = 0;
    char send_check = 0;
    char *send_order = NULL;

    send_order = (char *)malloc(23);
    memset(send_order,0,23);
    for(t = 0; t <= 8; t++)
    {
         *(send_order+t) = *(order+t);
    }
    *(send_order+9) = 0x00;
    *(send_order+10) = 0x00;
    *(send_order+11) = 0x00;
    *(send_order+12) = 0x05;
    *(send_order+13) = *(order+13);
    *(send_order+14) = *(order+14);                 //充电桩或显控程序
    *(send_order+15) = *(order+16);
    *(send_order+16) = *(order+17);
    if(send_state == 1)
    {
        *(send_order+17) = 0x01;         //发送成功
    }
    else
    {
        *(send_order+17) = 0x00;         //发送失败
    }
    for(t = 0 ; t <= 17 ; t++)
    {
        send_check ^=  *(send_order+t);
    }
    *(send_order+18) = send_check;  //异或校验位
    send_check = 0;
    *(send_order+19) = 0x3A;    //':'
    *(send_order+20) = 0x65;    //'e'
    *(send_order+21) = 0x6E;    //'n'
    *(send_order+22) = 0x64;    //'d'
    int i = sendto(UDP0_fd,send_order,23,0,(struct sockaddr *)&PC_addr,PC_addrlen);
    if(i <= 0){
        printf("Errno:%d %s\r\n",errno,strerror(errno));
    }

    for(i = 0;i < 23;i++){      //记录当前指令
        *(ResendText + i) = *(send_order + i);
    }
    IsResend = 1;               //开始UDP超时重发计时
    free(send_order);
    return 0;
}
/**************************************
函数名称：Program_finish_back
功能描述：程序升级完成最后一包发送到上位机
输入：
输出：
创建信息：
***************************************/
int Program_finish_back(char *order , int send_state)
{
    int t = 0;
    char send_check = 0;
    char *send_order = NULL;

    send_order = (char *)malloc(21);
    memset(send_order,0,21);
    for(t = 0; t <= 7; t++)
    {
        *(send_order+t) = *(order+t);
    }
    *(send_order+8) = 0x06;
    *(send_order+9) = 0x00;
    *(send_order+10) = 0x00;
    *(send_order+11) = 0x00;
    *(send_order+12) = 0x03;
    *(send_order+13) = *(order+13);
    *(send_order+14) = *(order+14);      //充电桩或显控程序
    if(send_state == 1)
    {
        *(send_order+15) = 0x01;         //发送成功
    }
    else
    {
        *(send_order+15) = 0x00;         //发送失败
    }
    for(t = 0 ; t <= 15 ; t++)
    {
        send_check ^=  *(send_order+t);
    }
    *(send_order+16) = send_check;  //异或校验位
    send_check = 0;
    *(send_order+17) = 0x3A;    //':'
    *(send_order+18) = 0x65;    //'e'
    *(send_order+19) = 0x6E;    //'n'
    *(send_order+20) = 0x64;    //'d'
    int i = sendto(UDP0_fd,send_order,21,0,(struct sockaddr *)&PC_addr,PC_addrlen);
    if(i<=0){
        printf("Errno:%d %s\r\n",errno,strerror(errno));
    }
    free(send_order);
    return 0;
}

/**************************************
函数名称：Configure_back
功能描述：配置完成
输入：
输出：
创建信息：
***************************************/
int Configure_back(char *order , int send_state)
{
    int t = 0;
    char send_check = 0;
    char *send_order = NULL;

    send_order = (char *)malloc(21);
    memset(send_order,0,21);
    for(t = 0; t <= 8; t++)
    {
        *(send_order+t) = *(order+t);
    }
    *(send_order+9) = 0x00;
    *(send_order+10) = 0x00;
    *(send_order+11) = 0x00;
    *(send_order+12) = 0x01;
    if(send_state == 1)
    {
        *(send_order+13) = 0x01;         //发送成功
    }
    else
    {
        *(send_order+13) = 0x00;         //发送失败
    }
    for(t = 0 ; t <= 13 ; t++)
    {
        send_check ^=  *(send_order+t);
    }
    *(send_order+14) = send_check;  //异或校验位
    *(send_order+15) = 0x3A;    //':'
    *(send_order+16) = 0x65;    //'e'
    *(send_order+17) = 0x6E;    //'n'
    *(send_order+18) = 0x64;    //'d'

    int i = sendto(UDP0_fd,send_order,19,0,(struct sockaddr *)&PC_addr,PC_addrlen);
    if(i<=0){
        printf("Errno:%d %s\r\n",errno,strerror(errno));
    }
    free(send_order);
    return 0;
}
