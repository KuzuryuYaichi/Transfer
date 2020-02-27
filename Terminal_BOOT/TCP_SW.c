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
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/msg.h>
#include "TCP.h"
#include "UDP.h"
#include "CRC.h"
#include "CJSON.h"
#include "Charger_can.h"
#include "MD5.h"
#include "Base64.h"
#include "Terminal.h"
#include "USART.h"
#include "CommonFunc.h"

static pthread_t pt_tid;
static int package_count = 0;
static double knees_mode = 0;

extern __u8 SITE_state;                  //数据终端状态
extern const char *PROGRAM_PATH;
extern int isDisconnected;
extern struct Terminal_Info Terminal;
extern struct Knee_struct Knee_can[EQUIP_NUM_MAX+1];
extern struct Net_struct Net_data[EQUIP_NUM_MAX+1];
extern struct CheckKnee_Struct CheckKnee[EQUIP_NUM_MAX+1];     //用于记录查询桩信息却不返回的桩体
extern struct UdpScan_struct   UpScan_Scan[EQUIP_NUM_MAX + 1];
extern struct UpData_struct UpData_data;

extern int Green_light;
extern int Red_light;
extern int isRunning;

/**************************************
函数名称：Protocol_make
功能描述：构造协议
输出：
创建信息：
***************************************/
static int Protocol_make(char *data,char *data_send,__u8 package_number,__u8 package_state,__u16 function_code)
{
    __u16 data_len = strlen(data);
    data_send[0] = 0x05;
    data_send[1] = 0x02;
    data_send[2] = (data_len+9) >> 8;   //数据长度的高字节
    data_send[3] = (data_len+9) & 0xFF;
    data_send[4] = 0x30;       //应用ID Byte_0
    data_send[5] = 0x30;       //应用ID Byte_1
    data_send[6] = 0x30;       //应用ID Byte_2
    data_send[7] = 0x32;       //应用ID Byte_3
    data_send[8] = package_number;    //包序号
    data_send[9] = package_state;     //包态
    data_send[10] = function_code >> 8;  //功能码高字节
    data_send[11] = function_code & 0xFF;         //功能码低字节
    data_send[12] = 0x00;      //保留
    __u16 t;
    for(t=0;t<data_len;t++)
    {
        data_send[13+t] = data[t];
    }
    __u16 crc = CRC16_CCITT_XMODEM(data_send+4,data_len+9);
    data_send[13+data_len] = crc >> 8;
    data_send[13+data_len+1] = crc & 0xFF;
    data_send[13+data_len+2] = 0x03;
    data_send[13+data_len+3] = 0x04;
    return 0;
}
/**************************************
函数名称：Site_load
功能描述：站点登录云端函数
输入：Site_id，站点ID。 *Site_password，站点密码（加盐、MD5）
输出：
创建信息：
***************************************/
static int Site_load(int sockfd,__u8 package_number,__u64 Site_id,char *Site_password)
{
    __u32 t = 0;
    __u16 functioncode = SITE_LOAD;

    char data_send[1024] = {0};
    char data_recv[1024] = {0};

    cJSON *load_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(load_json, "functioncode", functioncode);
    cJSON_AddNumberToObject(load_json, "site_id", Site_id);
    cJSON_AddStringToObject(load_json, "site_password", Site_password);
    char *load_data = cJSON_Print(load_json);
    cJSON_Delete(load_json);
    printf("load_data is:%s\r\n",load_data);

    Protocol_make(load_data,data_send,package_number,PACKAGE_state,functioncode);

    int send_len = write(sockfd,data_send,strlen(load_data)+17);
    free(load_data);

    //增加发送次数并计数
    printf("断线计数自增%d\r\n",++isDisconnected);
    if(isDisconnected > 10)
    {
        init_AT();
    }

    if(send_len > 0)
    {
        printf("waiting for receive ......\r\n");
        fd_set rd;
        FD_ZERO(&rd);
        FD_SET(sockfd,&rd);
        struct timeval timeout={3,0}; // 设置3s接收等待超时
        if(FD_ISSET(sockfd,&rd) && select(sockfd+1,&rd,NULL,NULL,&timeout)>0)
        {
            while(read(sockfd, data_recv, 1) > 0)
            {
                if(data_recv[0] != 0x05)
                {
                    continue;
                }
                read(sockfd, data_recv + 1, 1);
                if(data_recv[1] != 0x02)
                {
                    continue;
                }
                read(sockfd, data_recv + 2, 2);
                __u32 json_len = (data_recv[2]<<8) + data_recv[3] - 9;

                int recv_len = json_len + 13;
                while((recv_len -= read(sockfd, data_recv + 4 + (json_len + 13 - recv_len), recv_len))>0)
                {
                    struct timeval timeout={0,1000}; // 设置10ms接收等待超时
                    select(0,NULL,NULL,NULL,&timeout);
                }

                if(data_recv[15+json_len]!=0x03 || data_recv[16+json_len]!=0x04)
                {
                    printf("Tail Error!!\r\n");
                    return -1;
                }
                __u16 crc = CRC16_CCITT_XMODEM(data_recv+4,json_len+9);
                if((crc>>8)!=data_recv[13+json_len] || (crc&0xFF)!=data_recv[14+json_len])
                {
                    printf("CRC error!!\r\n");
                    return -1;
                }
                char data_recv_json[1024] = {0};
                for(t=0;t<json_len;t++)
                {
                    data_recv_json[t] = data_recv[13+t];
                }
                printf("receive finish.\r\n");
                printf("data_recv_json is : %s\r\n",data_recv_json);

                functioncode = (data_recv[10]<<8) + data_recv[11];

                cJSON *Load_recv = cJSON_Parse(data_recv_json);
                cJSON *Load_site_login_status = cJSON_GetObjectItem(Load_recv,"site_login_status");
                if(!Load_site_login_status || Load_site_login_status->valueint != 1)    //验证登录是否成功
                {
                    cJSON_Delete(Load_recv);
                    return -1;
                }

                cJSON *Load_site_comm_token = cJSON_GetObjectItem(Load_recv,"site_comm_token");
                if(Load_site_comm_token->valuestring != NULL)
                {
                    strcpy(Terminal.TOKEN_site,Load_site_comm_token->valuestring);  //更新站点token
                }
                cJSON_Delete(Load_recv);
                break;
            }
        }
        else
        {
            return -1;
        }
    }
    printf("Login Success\r\n");
    isDisconnected = -1;
    //通知桩体网络连接恢复
    struct Terminal_MsgSt NetRecv_queue = {0};
    NetRecv_queue.MsgType = IPC_NOWAIT;
    NetRecv_queue.MsgBuff[1] = NET_DISCONNECT_EXIT;
    msgsnd(UpdataRecv_qid, &NetRecv_queue, 2, IPC_NOWAIT);
    //指示灯变化
    Red_light = LIGHT_OFF;
    return 0;
}
/**************************************
函数名称：   serve
功能描述：   站点与云端通讯主函数
输入：      sockfd
输出：
创建信息：
***************************************/
static int serve(int sockfd)
{
    int load_time = 0;
    int load_state = -1;

    if(SITE_state == NO_LOAD)
    {
        char salt_tmp[16] = {0};
        FILE *SALT_fp = NULL;
        while((SALT_fp = fopen("/salt.txt","rb+")) == NULL)
        {
            printf("Open SALT_fp Fail\r\n");
        }
        fseek(SALT_fp,0,SEEK_SET);
        fread(salt_tmp,1,6,SALT_fp);
        fclose(SALT_fp);
        printf("当前登录盐值：%s\r\n",Terminal.SALT_site);
        do{
            struct timeval timeout={0,10}; // 设置10ms接收等待超时
            select(0,NULL,NULL,NULL,&timeout);

            char site_pwd_tmp1[1024] = {0};
            char site_pwd_tmp2[1024] = {0};
            char site_pwd[1024] = {0};
            strcpy(site_pwd_tmp1,Terminal.SALT_password);
            //strcat(site_pwd_tmp1,salt_tmp);       //加盐
            //strcat(site_pwd_tmp1,SALT_site);      //加盐(固定盐值)
            strcat(site_pwd_tmp1,"6D+#wllurZQ0");
            MD5Digest(site_pwd_tmp1,strlen(site_pwd_tmp1),site_pwd_tmp2);	//MD5加密

            StrToHex(site_pwd_tmp2,16,site_pwd);
            load_state = Site_load(sockfd,1,Terminal.SITE_ID,site_pwd);
            load_time++;
            if(load_time > 1)
            {
                load_time = 0;
                SITE_state = NO_LOAD;
                printf("Load Fail !!\r\n");
                return -1;
            }
        }while(load_state == -1);
        SITE_state = LOAD_ON;
        printf("Load OK\r\n");
        Green_light = LIGHT_BLINK;
    }
}
extern __u8 SendFlag;
/**************************************
函数名称：	NetSendData()
功能描述：	以太网数据发送
输入：
输出：
创建信息:
***************************************/
static int NetSendData(int sockfd)
{
    __u16 functioncode;
    cJSON *data_json = NULL;
    char *spec_data = NULL;
    char data_send[1024] = {0};

    struct Terminal_MsgSt NetSend_queue ={0};
    NetSend_queue.MsgType = IPC_NOWAIT;

    int temp = msgrcv(CanRecv_qid, &NetSend_queue, 2, 0, IPC_NOWAIT);
    if(SITE_state == NO_LOAD)return 0;
    if(temp >= 0)
    {
        __u32 function_code = NetSend_queue.MsgBuff[1]; //车桩指令编码
        __u16 equip_num = NetSend_queue.MsgBuff[0];     //桩位号
        data_json = cJSON_CreateObject();        //等待发送的数据结构
        if(equip_num<=EQUIP_NUM_MAX)pthread_mutex_lock(&Knee_can[equip_num].mt);
        switch(function_code)
        {
            case ERROR_APP:
            {
                printf("APP 错误\r\n");
                functioncode = APP_ERROR_BACK;
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "site_id", Terminal.SITE_ID);
                cJSON_AddNumberToObject(data_json, "update_dev", 1);
                break;
            }
            case GET_CONFIG_RESULT:
            {
                functioncode = GET_CONFIG_BACK;
                cJSON_AddNumberToObject(data_json, "return_result", 1);
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "site_id", Terminal.SITE_ID);
                cJSON_AddStringToObject(data_json, "site_password", Terminal.SALT_password);
                cJSON_AddStringToObject(data_json, "login_ip", Terminal.Server_IP);
                cJSON_AddNumberToObject(data_json, "login_port", Terminal.Server_Port);
                cJSON_AddStringToObject(data_json, "net_wifiname", Terminal.Wifi_name);
                cJSON_AddStringToObject(data_json, "net_wifipsd", Terminal.Wifi_password);
                cJSON_AddNumberToObject(data_json, "reset_count", Terminal.ResetCount);
                cJSON_AddNumberToObject(data_json, "succ_count", Terminal.ConnectCount);
                cJSON_AddNumberToObject(data_json, "fail_count", Terminal.DisConnectCount);
                cJSON_AddNumberToObject(data_json, "knee_mode", knees_mode);
                cJSON_AddNumberToObject(data_json, "backstage_sel", Terminal.BackStage_Sel);
                break;
            }
            case SET_CONFIG_RESULT:
            {
                functioncode = SET_CONFIG_BACK;
                cJSON_AddNumberToObject(data_json, "return_result", 1);
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "site_id", Terminal.SITE_ID);
                cJSON_AddStringToObject(data_json, "site_password", Terminal.SALT_password);
                cJSON_AddStringToObject(data_json, "login_ip", Terminal.Server_IP);
                cJSON_AddNumberToObject(data_json, "login_port", Terminal.Server_Port);
                cJSON_AddStringToObject(data_json, "net_wifiname", Terminal.Wifi_name);
                cJSON_AddStringToObject(data_json, "net_wifipsd", Terminal.Wifi_password);
                cJSON_AddNumberToObject(data_json, "reset_count", Terminal.ResetCount);
                cJSON_AddNumberToObject(data_json, "succ_count", Terminal.ConnectCount);
                cJSON_AddNumberToObject(data_json, "fail_count", Terminal.DisConnectCount);
                cJSON_AddNumberToObject(data_json, "knee_mode", knees_mode);
                cJSON_AddNumberToObject(data_json, "backstage_sel", Terminal.BackStage_Sel);
                break;
            }
            case BOOTLOAD_IN:
            {
                functioncode = ENTER_BOOTLOADER_BACK;
                cJSON_AddNumberToObject(data_json, "return_result", 1);
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "site_id", Terminal.SITE_ID);
                break;
            }
            case BOOTLOAD_OUT:
            {
                functioncode = ENTER_BOOTLOADER_BACK;
                cJSON_AddNumberToObject(data_json, "return_result", 1);
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "site_id", Terminal.SITE_ID);
                isRunning = PROGRAM_VIRTUAL_SERVER;
                break;
            }
            case EQUIP_ACCEPT_BACK:
            {
                printf("终端扫描返回\r\n");
                functioncode = SCAN_VERSION_BACK;
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "return_result", 1);
                cJSON_AddNumberToObject(data_json, "update_dev", 1);
                cJSON_AddStringToObject(data_json, "soft_version", Terminal.Software_version);
                cJSON_AddStringToObject(data_json, "hard_version", Terminal.Hardware_version);
                cJSON_AddNumberToObject(data_json, "device_id", Terminal.SITE_ID);//返回站点ID
                cJSON_AddNumberToObject(data_json, "knee_loc", 0);
                break;
            }
            case EQUIP_MCU_BACK:
            {
                printf("主控扫描返回\r\n");
                functioncode = SCAN_VERSION_BACK;
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "return_result", 1);
                cJSON_AddNumberToObject(data_json, "update_dev", 2);
                char ver[10] = {0};
                sprintf(ver,"%08x",UpScan_Scan[equip_num].MS);
                cJSON_AddStringToObject(data_json, "soft_version", ver);
                sprintf(ver,"%08x",UpScan_Scan[equip_num].MH);
                cJSON_AddStringToObject(data_json, "hard_version", ver);
                cJSON_AddNumberToObject(data_json, "device_id", Knee_can[equip_num].knee_ID);//返回桩体ID
                cJSON_AddNumberToObject(data_json, "knee_loc", equip_num);
                break;
            }
            case EQUIP_DIS_BACK:
            {
                printf("显控扫描返回\r\n");
                functioncode = SCAN_VERSION_BACK;
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "return_result", 1);
                cJSON_AddNumberToObject(data_json, "update_dev", 3);
                char ver[10] = {0};
                sprintf(ver,"%08x",UpScan_Scan[equip_num].DS);
                cJSON_AddStringToObject(data_json, "soft_version", ver);
                sprintf(ver,"%08x",UpScan_Scan[equip_num].DH);
                cJSON_AddStringToObject(data_json, "hard_version", ver);
                cJSON_AddNumberToObject(data_json, "device_id", Knee_can[equip_num].knee_ID);//返回桩体ID
                cJSON_AddNumberToObject(data_json, "knee_loc", equip_num);
                break;
            }
            case EQUIP_CARRIER_BACK:
            {
                printf("载波扫描返回\r\n");
                functioncode = SCAN_VERSION_BACK;
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "return_result", 1);
                cJSON_AddNumberToObject(data_json, "update_dev", 4);
                char ver[10] = {0};
                sprintf(ver,"%08x",UpScan_Scan[equip_num].CS);
                cJSON_AddStringToObject(data_json, "soft_version", ver);
                sprintf(ver,"%08x",UpScan_Scan[equip_num].CH);
                cJSON_AddStringToObject(data_json, "hard_version", ver);
                cJSON_AddNumberToObject(data_json, "device_id", Knee_can[equip_num].knee_ID);//返回桩体ID
                cJSON_AddNumberToObject(data_json, "knee_loc", equip_num);
                break;
            }
            case EQUIP_LOGO_BACK:
            {
                printf("字库扫描返回\r\n");
                functioncode = SCAN_VERSION_BACK;
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "return_result", 1);
                cJSON_AddNumberToObject(data_json, "update_dev", 5);
                char ver[10] = {0};
                sprintf(ver,"%08x",UpScan_Scan[equip_num].LS);
                cJSON_AddStringToObject(data_json, "soft_version", ver);
                sprintf(ver,"%08x",UpScan_Scan[equip_num].LH);
                cJSON_AddStringToObject(data_json, "hard_version", ver);
                cJSON_AddNumberToObject(data_json, "device_id", Knee_can[equip_num].knee_ID);//返回桩体ID
                cJSON_AddNumberToObject(data_json, "knee_loc", equip_num);
                break;
            }
            case EQUIP_PCRD_BACK:
            {
                printf("断电复位器扫描返回\r\n");
                functioncode = SCAN_VERSION_BACK;
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "return_result", 1);
                cJSON_AddNumberToObject(data_json, "update_dev", 6);
                char ver[10] = {0};
                sprintf(ver,"%08x",UpScan_Scan[equip_num].PS);
                cJSON_AddStringToObject(data_json, "soft_version", ver);
                sprintf(ver,"%08x",UpScan_Scan[equip_num].PH);
                cJSON_AddStringToObject(data_json, "hard_version", ver);
                cJSON_AddNumberToObject(data_json, "device_id", Knee_can[equip_num].knee_ID);//返回桩体ID
                cJSON_AddNumberToObject(data_json, "knee_loc", equip_num);
                break;
            }
            case PROGRAM_WRITE_BACK:
            {
                printf("程序下载返回\r\n");
                functioncode = UPDATE_DOWNLOAD_BACK;
                cJSON_AddNumberToObject(data_json, "return_result", equip_num);//此时equip_num作为程序烧写结果
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "site_id", Terminal.SITE_ID);
                cJSON_AddNumberToObject(data_json, "frame_number", package_count);
                break;
            }
            case UPDATE_WRITE_CAN_BACK:
            {
                functioncode = UPDATE_WRITE_BACK;
                cJSON_AddNumberToObject(data_json, "update_progress", equip_num);//此时equip_num作为程序烧写进度
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                if(equip_num)
                    cJSON_AddNumberToObject(data_json, "device_id", Knee_can[equip_num].knee_ID);//返回桩体ID
                else
                    cJSON_AddNumberToObject(data_json, "device_id", Terminal.SITE_ID);//返回站点ID
                break;
            }
            case UPDATE_WRITE_CAN_FINISH:
            {
                functioncode = UPDATE_WRITE_BACK_FINISH;
                cJSON_AddNumberToObject(data_json, "return_result", 1);//此时equip_num作为程序烧写结果
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                if(equip_num)
                    cJSON_AddNumberToObject(data_json, "device_id", Knee_can[equip_num].knee_ID);//返回桩体ID
                else
                    cJSON_AddNumberToObject(data_json, "device_id", Terminal.SITE_ID);//返回站点ID
                break;
            }
            default:
            {
                break;
            }
        }
        if(equip_num<=EQUIP_NUM_MAX)pthread_mutex_unlock(&Knee_can[equip_num].mt);
        /*****************************************************************/
        spec_data = cJSON_Print(data_json);
        cJSON_Delete(data_json);
        if(spec_data)
        {
            printf("spec_data is:%s\r\n",spec_data);
            Protocol_make(spec_data,data_send,1,PACKAGE_state,functioncode);
            if(write(sockfd,data_send,strlen(spec_data)+17) < 0)perror("Send");
            free(spec_data);
        }
        else printf("cJSON malloc error");
        /*****************************************************************/
    }
}
/**************************************
函数名称：RecvNetData()
功能描述：接收以太网数据
输入：
输出：
创建信息:
***************************************/
static int RecvNetData(int sockfd)
{
    if(SITE_state == NO_LOAD)return 0;
    __u32 t;
    int recv_len;
    __u64 functioncode;
    char data_recv[0x800] = {0};

    struct Terminal_MsgSt NetRecv_queue = {0};
    NetRecv_queue.MsgType = IPC_NOWAIT;

    struct timeval timeout={0,1000}; // 设置10ms接收等待超时
    fd_set rd;
    FD_ZERO(&rd);
    FD_SET(sockfd,&rd);
    if(FD_ISSET(sockfd,&rd) && select(sockfd+1,&rd,NULL,NULL,&timeout)>0)
    {
        while((recv_len = read(sockfd,data_recv,1)) > 0)
        {
            if(data_recv[0] != 0x05)
            {
                continue;
            }
            read(sockfd, data_recv + 1, 1);
            if(data_recv[1] != 0x02)
            {
                continue;
            }
            read(sockfd, data_recv + 2, 2);
            __u32 json_len = (data_recv[2]<<8) + data_recv[3] - 9;
            recv_len = json_len + 13;
            printf("Recv Len:%d\r\n",recv_len);
            while((recv_len -= read(sockfd, data_recv + 4 + (json_len + 13 - recv_len), recv_len))>0)
            {
                struct timeval timeout={0,1000}; // 设置10ms接收等待超时
                select(0,NULL,NULL,NULL,&timeout);
            }
            if(data_recv[15+json_len]!=0x03 || data_recv[16+json_len]!=0x04)
            {
                printf("Tail Error!!\r\n");
                continue;
            }
            __u16 crc = CRC16_CCITT_XMODEM(data_recv+4,json_len+9);
            if((crc>>8)!=data_recv[13+json_len] || (crc&0xFF)!=data_recv[14+json_len])
            {
                printf("CRC error!!\r\n");
                continue;
            }
            functioncode = (data_recv[10]<<8) + data_recv[11];

            char data_recv_json[1024] = {0};
            printf("data_recv_json is:");
            for(t=0;t<json_len;t++)
            {
                data_recv_json[t] = data_recv[13+t];
                if(functioncode != UPDATE_DOWNLOAD || t<json_len-512)printf("%c",data_recv[13+t]);
            }
            SendFlag = 0;
            cJSON *spec_data = cJSON_Parse(data_recv_json);
            switch(functioncode)
            {
                case GET_CONFIG:
                {
                    File_Configuration();//读取配置文件
                    NetRecv_queue.MsgBuff[1] = GET_CONFIG_RESULT;
                    msgsnd(CanRecv_qid, &NetRecv_queue, 2, IPC_NOWAIT);
                    break;
                }
                case SET_CONFIG:
                {
                    cJSON *site_id = cJSON_GetObjectItem(spec_data,"site_id");
                    cJSON *site_password = cJSON_GetObjectItem(spec_data,"site_password");
                    cJSON *login_ip = cJSON_GetObjectItem(spec_data,"login_ip");
                    cJSON *login_port = cJSON_GetObjectItem(spec_data,"login_port");
                    cJSON *reset_count = cJSON_GetObjectItem(spec_data,"reset_count");
                    cJSON *succ_count = cJSON_GetObjectItem(spec_data,"succ_count");
                    cJSON *fail_count = cJSON_GetObjectItem(spec_data,"fail_count");
                    cJSON *net_wifiname = cJSON_GetObjectItem(spec_data,"net_wifiname");
                    cJSON *net_wifipsd = cJSON_GetObjectItem(spec_data,"net_wifipsd");
                    cJSON *knee_mode = cJSON_GetObjectItem(spec_data,"knee_mode");
                    cJSON *backstage_sel = cJSON_GetObjectItem(spec_data,"backstage_sel");

                    if(site_id)site_id_reset(&site_id->valuedouble,SITE_ID_WRITE);
                    if(site_password)site_id_reset(site_password->valuestring,SITE_PASSWORD_WRITE);
                    if(login_ip)site_id_reset(login_ip->valuestring,SERVER_IP_WRITE);
                    if(login_port)site_id_reset(&login_port->valueint,SERVER_PORT_WRITE);
                    if(net_wifiname)site_id_reset(net_wifiname->valuestring,WIFI_NAME_WRITE);
                    if(net_wifipsd)site_id_reset(net_wifipsd->valuestring,WIFI_PASSWORD_WRITE);
                    if(reset_count)site_id_reset(&reset_count->valuedouble,RESET_COUNT_WRITE);
                    if(succ_count)site_id_reset(&succ_count->valuedouble,SUCC_COUNT_WRITE);
                    if(fail_count)site_id_reset(&fail_count->valuedouble,FAIL_COUNT_WRITE);
                    if(backstage_sel)site_id_reset(&backstage_sel->valueint,BACKSTAGE_SELECT);

                    File_Configuration();//读取配置文件
                    NetRecv_queue.MsgBuff[1] = SET_CONFIG_RESULT;
                    msgsnd(CanRecv_qid, &NetRecv_queue, 2, IPC_NOWAIT);

//                    if(knee_mode)
//                    {
//                        knees_mode = knee_mode->valuedouble;
//                        if(knees_mode == 1)
//                        {
//                            printf("网络版\r\n");
//                            NetRecv_queue.MsgBuff[1] = NET;
//                        }
//                        else if(knees_mode == 2)
//                        {
//                            printf("单机版\r\n");
//                            NetRecv_queue.MsgBuff[1] = SINGLE;
//                        }
//                    }
//                    msgsnd(UpdataRecv_qid, &NetRecv_queue, 2, IPC_NOWAIT);

                    break;
                }
                case EXIT_VIRTUAL_SERVER:
                {
                    printf("退出虚拟服务器\r\n");
                    isRunning = PROGRAM_ACTUAL_SERVER;  //跳转到真实服务器
                    break;
                }
                case ENTER_BOOTLOADER:
                {
                    cJSON *enter_bld = cJSON_GetObjectItem(spec_data,"enter_bld");
                    if(enter_bld && enter_bld->valueint == 0)
                    {
                        printf("退出BOOTLOADER 进入APP与虚拟服务器连接\r\n");
                        NetRecv_queue.MsgBuff[1] = BOOTLOAD_OUT;
                        msgsnd(CanRecv_qid, &NetRecv_queue, 2, IPC_NOWAIT);
                    }
                    else if(enter_bld && enter_bld->valueint == 1)
                    {
                        printf("进入BOOTLOADER\r\n");
                        NetRecv_queue.MsgBuff[1] = BOOTLOAD_IN;
                        msgsnd(CanRecv_qid, &NetRecv_queue, 2, IPC_NOWAIT);
                    }
                    else printf("非法指令\r\n");
                    break;
                }
                case SCAN_VERSION:
                {
                    printf("扫描");
                    cJSON *scan_dev_p = cJSON_GetObjectItem(spec_data,"update_dev");
                    if(scan_dev_p)
                    {
                        switch(scan_dev_p->valueint)
                        {
                            case 1:
                            {
                                printf("终端\r\n");
                                NetRecv_queue.MsgBuff[1] = EQUIP_ACCEPT_BACK;
                                msgsnd(CanRecv_qid, &NetRecv_queue, 2, IPC_NOWAIT);
                                break;
                            }
                            case 2:
                            {
                                printf("主控\r\n");
                                NetRecv_queue.MsgBuff[1] = EQUIP_MCU;
                                break;
                            }
                            case 3:
                            {
                                printf("显控\r\n");
                                NetRecv_queue.MsgBuff[1] = EQUIP_DIS;
                                break;
                            }
                            case 4:
                            {
                                printf("载波\r\n");
                                NetRecv_queue.MsgBuff[1] = EQUIP_CARRIER;
                                break;
                            }
                            case 5:
                            {
                                printf("字库LOGO\r\n");
                                NetRecv_queue.MsgBuff[1] = EQUIP_LOGO;
                                break;
                            }
                            case 6:
                            {
                                printf("断电复位器\r\n");
                                NetRecv_queue.MsgBuff[1] = EQUIP_PCRD;
                                break;
                            }
                            default:printf("非法参数\r\n");break;
                        }
                    }
                }
                case UPDATE_DOWNLOAD:
                {
                    cJSON *program_size = cJSON_GetObjectItem(spec_data,"program_size");
                    cJSON *frame_number = cJSON_GetObjectItem(spec_data,"frame_number");
                    if(program_size && frame_number)
                    {
                        UpData_data.program_size = (__u64)program_size->valuedouble;
                        if(frame_number->valueint == 0)
                        {
                            if(!remove(PROGRAM_PATH))printf("updateFile删除成功\n");
                            package_count = -1;
                        }
                        if(frame_number->valueint == package_count + 1)
                        {
                            int packet_total = program_size->valueint/512 + (program_size->valueint%512?1:0);//包总数
                            printf("packet_total:%d\r\n",packet_total);

                            if(frame_number->valueint >= packet_total)printf("超出程序界限\r\n");
                            else
                            {
                                package_count = frame_number->valueint;
                                NetRecv_queue.MsgBuff[0] = Terminal_Updata(data_recv_json + json_len - 512,packet_total-1==frame_number->valueint?program_size->valueint%512:512,PROGRAM_PATH);
                                NetRecv_queue.MsgBuff[1] = PROGRAM_WRITE_BACK;
                                msgsnd(CanRecv_qid, &NetRecv_queue, 2, IPC_NOWAIT);
                            }
                        }
                        else
                        {
                            printf("包计数错误\r\n");
                            NetRecv_queue.MsgBuff[0] = 1;
                            NetRecv_queue.MsgBuff[1] = PROGRAM_WRITE_BACK;
                            msgsnd(CanRecv_qid, &NetRecv_queue, 2, IPC_NOWAIT);
                        }
                    }
                    else printf("参数不完全\r\n");
                    break;
                }
                case UPDATE_WRITE:
                {
                    printf("升级 ");
                    cJSON *device_id = cJSON_GetObjectItem(spec_data,"device_id");
                    cJSON *update_dev = cJSON_GetObjectItem(spec_data,"update_dev");
                    cJSON *knee_loc = cJSON_GetObjectItem(spec_data,"knee_loc");
                    if(device_id && update_dev && knee_loc)
                    {
                        NetRecv_queue.MsgBuff[1] = UPDATE_WRITE_CAN_BACK;
                        NetRecv_queue.MsgBuff[0] = 0;
                        msgsnd(CanRecv_qid, &NetRecv_queue, 2, IPC_NOWAIT);

                        UpData_data.equip_name = update_dev->valueint;
                        switch (update_dev->valueint) {
                        case 1:
                        {
                            printf("终端程序\r\n");
                            ClearFlagAPP();//清除APPFLAG
                            if(chmod(PROGRAM_PATH,S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH)<0)perror("chmod失败\r\n");
                            if(pthread_create(&pt_tid,NULL,ProgramTransfer_thread,NULL))
                            {
                                perror("Create NET thread error!");
                                return;
                            }
                            break;
                        }
                        case 2:
                        {
                            printf("主控程序\r\n");
                            NetRecv_queue.MsgBuff[0] = knee_loc->valueint;
                            NetRecv_queue.MsgBuff[1] = BOOT_IN;
                            break;
                        }
                        case 3:
                        {
                            printf("显控程序\r\n");
                            NetRecv_queue.MsgBuff[0] = knee_loc->valueint;
                            NetRecv_queue.MsgBuff[1] = BOOT_IN;
                            break;
                        }
                        case 4:
                        {
                            printf("载波程序\r\n");
                            NetRecv_queue.MsgBuff[0] = knee_loc->valueint;
                            NetRecv_queue.MsgBuff[1] = BOOT_IN;
                            break;
                        }
                        case 5:
                        {
                            printf("字库LOGO\r\n");
                            NetRecv_queue.MsgBuff[0] = knee_loc->valueint;
                            NetRecv_queue.MsgBuff[1] = BOOT_IN;
                            break;
                        }
                        case 6:
                        {
                            printf("断电复位器\r\n");
                            NetRecv_queue.MsgBuff[0] = knee_loc->valueint;
                            NetRecv_queue.MsgBuff[1] = BOOT_IN;
                            break;
                        }
                        default:
                        {
                            printf("设备类型错误\r\n");
                            break;
                        }
                        }
                    }
                    else printf("参数不完全\r\n");
                    break;
                }
                default:
                {
                    break;
                }
            }
            cJSON_Delete(spec_data);
            printf("msgsnd temp is:%d\r\n",msgsnd(UpdataRecv_qid, &NetRecv_queue, 2, IPC_NOWAIT));
            break;
        }
    }
}

void TCP_SW(int sockfd)
{
    serve(sockfd);
    NetSendData(sockfd);
    RecvNetData(sockfd);
}
