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

extern __u8 SITE_state;                  //�����ն�״̬
extern const char *PROGRAM_PATH;
extern int isDisconnected;
extern struct Terminal_Info Terminal;
extern struct Knee_struct Knee_can[EQUIP_NUM_MAX+1];
extern struct Net_struct Net_data[EQUIP_NUM_MAX+1];
extern struct CheckKnee_Struct CheckKnee[EQUIP_NUM_MAX+1];     //���ڼ�¼��ѯ׮��Ϣȴ�����ص�׮��
extern struct UdpScan_struct   UpScan_Scan[EQUIP_NUM_MAX + 1];
extern struct UpData_struct UpData_data;

extern int Green_light;
extern int Red_light;
extern int isRunning;

/**************************************
�������ƣ�Protocol_make
��������������Э��
�����
������Ϣ��
***************************************/
static int Protocol_make(char *data,char *data_send,__u8 package_num,__u8 package_state,__u16 function_code)
{
    __u16 data_len = strlen(data);
    data_send[0] = 0x40;
    data_send[1] = 0x3A;
    data_send[2] = (data_len+35) >> 8;
    data_send[3] = (data_len+35) & 0xFF;
    memcpy(data_send+4,Terminal.TOKEN_site,32);
    data_send[36] = package_num;                  //�����
    data_send[37] = function_code >> 8;    //��������ֽ�
    data_send[38] = function_code & 0xFF;         //��������ֽ�
    __u16 t;
    for(t=0;t<data_len;t++)
    {
        data_send[39+t] = data[t];
    }
    __u16 crc = CRC16_CCITT_XMODEM(data_send+4,data_len+35);
    data_send[39+data_len] = crc >> 8;
    data_send[39+data_len+1] = crc & 0xFF;
    data_send[39+data_len+2] = 0x0D;
    data_send[39+data_len+3] = 0x0A;
    return 0;
}
/**************************************
�������ƣ�   Site_load
����������   վ���¼�ƶ˺���
���룺       Site_id��վ��ID�� *Site_password��վ�����루���Ρ�MD5��
�����
������Ϣ��
***************************************/
static int Site_load(int sockfd,__u8 package_number,__u64 Site_id,char *Site_password)
{
    __u32 t = 0;
    __u16 functioncode = SITE_LOAD;

    char data_send[1024] = {0};
    char data_recv[1024] = {0};

    cJSON *load_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(load_json, "sid", Site_id);
    cJSON_AddStringToObject(load_json, "pwd", Site_password);
    char *load_data = cJSON_Print(load_json);
    cJSON_Delete(load_json);

    printf("load_data is:%s\r\n",load_data);
    Protocol_make(load_data,data_send,package_number,PACKAGE_state,functioncode);
    int send_len = write(sockfd,data_send,strlen(load_data)+43);
    free(load_data);

    //���ӷ��ʹ���������
    printf("���߼�������%d\r\n",++isDisconnected);
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
        struct timeval timeout={3,0};
        if(FD_ISSET(sockfd,&rd) && select(sockfd+1,&rd,NULL,NULL,&timeout)>0)
        {
            while(read(sockfd, data_recv, 1) > 0)
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

                int recv_len = json_len + 35 + 4;
                while((recv_len -= read(sockfd, data_recv + 4 + (json_len + 35 + 4 - recv_len), recv_len))>0)
                {
                    struct timeval timeout={0,100}; //����10ms���յȴ���ʱ
                    select(0,NULL,NULL,NULL,&timeout);
                }
                if(data_recv[41+json_len]!=0x0D || data_recv[42+json_len]!=0x0A)
                {
                    printf("Tail Error!!\r\n");
                    return -1;
                }
                __u16 crc = CRC16_CCITT_XMODEM(data_recv+4,json_len+35);
                if((crc>>8) != data_recv[39+json_len] || (crc&0xFF) != data_recv[40+json_len])
                {
                    printf("CRC error!!\r\n");
                    return -1;
                }
                char data_recv_json[1024] = {0};
                for(t=0;t<json_len;t++)
                {
                    data_recv_json[t] = data_recv[39+t];
                }
                printf("receive finish.\r\n");
                printf("data_recv_json is : %s\r\n",data_recv_json);

                memcpy(Terminal.TOKEN_site,data_recv+4,32);
                functioncode = (data_recv[37]<<8) + data_recv[38];

                cJSON *Load_recv = cJSON_Parse(data_recv_json);
                cJSON *Load_site_login_status = cJSON_GetObjectItem(Load_recv,"sls");
                if(!Load_site_login_status || Load_site_login_status->valueint) //��֤��¼�Ƿ�ɹ�
                {
                    cJSON_Delete(Load_recv);
                    return -1;
                }

                cJSON *Load_site_comm_token = cJSON_GetObjectItem(Load_recv,"site_comm_token");
                if(Load_site_comm_token && Load_site_comm_token->valuestring)
                {
                    strcpy(Terminal.TOKEN_site,Load_site_comm_token->valuestring);           //����վ��token
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
    //֪ͨ׮���������ӻָ�
    struct Terminal_MsgSt NetRecv_queue = {0};
    NetRecv_queue.MsgType = IPC_NOWAIT;
    NetRecv_queue.MsgBuff[1] = NET_DISCONNECT_EXIT;
    msgsnd(UpdataRecv_qid, &NetRecv_queue, 2, IPC_NOWAIT);
    //ָʾ�Ʊ仯
    Red_light = LIGHT_OFF;
    return 0;
}
/**************************************
�������ƣ�serve
����������վ�����ƶ�ͨѶ������
���룺sockfd
�����
������Ϣ��
***************************************/
static int serve(int sockfd)
{
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
        printf("��ǰ��¼��ֵ��%s\r\n",Terminal.SALT_site);

        char site_pwd_tmp1[1024] = {0};
        char site_pwd_tmp2[1024] = {0};
        char site_pwd[1024] = {0};
        strcpy(site_pwd_tmp1,Terminal.SALT_password);
        MD5Digest(site_pwd_tmp1,strlen(site_pwd_tmp1),site_pwd_tmp2);	//MD5����
        StrToHex(site_pwd_tmp2,16,site_pwd);
        if(Site_load(sockfd,1,Terminal.SITE_ID,site_pwd) == -1)
        {
            SITE_state = NO_LOAD;
            printf("Load Fail !!\r\n");
            return -1;
        }
        SITE_state = LOAD_ON;
        printf("Load OK\r\n");
        Green_light = LIGHT_BLINK;
    }
}
extern __u8 SendFlag;
/**************************************
�������ƣ�NetSendData()
������������̫�����ݷ���
���룺
�����
������Ϣ:
***************************************/
static int NetSendData(int sockfd)
{
    __u16 functioncode = 0;
    struct Terminal_MsgSt NetSend_queue ={0};
    NetSend_queue.MsgType = IPC_NOWAIT;

    int temp = msgrcv(CanRecv_qid, &NetSend_queue, 2, 0, IPC_NOWAIT);
    if(SITE_state == NO_LOAD)return 0;
    if(temp >= 0)
    {
        __u32 function_code = NetSend_queue.MsgBuff[1]; //��׮ָ�����
        __u16 equip_num = NetSend_queue.MsgBuff[0];     //׮λ��
        cJSON *data_json = cJSON_CreateObject();        //�ȴ����͵����ݽṹ
        if(equip_num <= EQUIP_NUM_MAX)pthread_mutex_lock(&Knee_can[equip_num].mt);
        printf("equip_num:%d\r\n",equip_num);
        switch(function_code)
        {
            case ERROR_APP:
            {
                printf("APP ����\r\n");
                functioncode = APP_ERROR_BACK;
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "site_id", Terminal.SITE_ID);
                cJSON_AddNumberToObject(data_json, "return_result", 1);
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
                printf("�ն�ɨ�践��\r\n");
                functioncode = SCAN_VERSION_BACK;
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "return_result", 1);
                cJSON_AddNumberToObject(data_json, "update_dev", 1);
                cJSON_AddStringToObject(data_json, "soft_version", Terminal.Software_version);
                cJSON_AddStringToObject(data_json, "hard_version", Terminal.Hardware_version);
                cJSON_AddNumberToObject(data_json, "device_id", Terminal.SITE_ID);//����վ��ID
                cJSON_AddNumberToObject(data_json, "knee_loc", 0);
                break;
            }
            case EQUIP_MCU_BACK:
            {
                printf("����ɨ�践��\r\n");
                functioncode = SCAN_VERSION_BACK;
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "return_result", 1);
                cJSON_AddNumberToObject(data_json, "update_dev", 2);
                char ver[10] = {0};
                sprintf(ver,"%08x",UpScan_Scan[equip_num].MS);
                cJSON_AddStringToObject(data_json, "soft_version", ver);
                sprintf(ver,"%08x",UpScan_Scan[equip_num].MH);
                cJSON_AddStringToObject(data_json, "hard_version", ver);
                cJSON_AddNumberToObject(data_json, "device_id", Knee_can[equip_num].knee_ID);//����׮��ID
                cJSON_AddNumberToObject(data_json, "knee_loc", equip_num);
                break;
            }
            case EQUIP_DIS_BACK:
            {
                printf("�Կ�ɨ�践��\r\n");
                functioncode = SCAN_VERSION_BACK;
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "return_result", 1);
                cJSON_AddNumberToObject(data_json, "update_dev", 3);
                char ver[10] = {0};
                sprintf(ver,"%08x",UpScan_Scan[equip_num].DS);
                cJSON_AddStringToObject(data_json, "soft_version", ver);
                sprintf(ver,"%08x",UpScan_Scan[equip_num].DH);
                cJSON_AddStringToObject(data_json, "hard_version", ver);
                cJSON_AddNumberToObject(data_json, "device_id", Knee_can[equip_num].knee_ID);//����׮��ID
                cJSON_AddNumberToObject(data_json, "knee_loc", equip_num);
                break;
            }
            case EQUIP_CARRIER_BACK:
            {
                printf("�ز�ɨ�践��\r\n");
                functioncode = SCAN_VERSION_BACK;
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "return_result", 1);
                cJSON_AddNumberToObject(data_json, "update_dev", 4);
                char ver[10] = {0};
                sprintf(ver,"%08x",UpScan_Scan[equip_num].CS);
                cJSON_AddStringToObject(data_json, "soft_version", ver);
                sprintf(ver,"%08x",UpScan_Scan[equip_num].CH);
                cJSON_AddStringToObject(data_json, "hard_version", ver);
                cJSON_AddNumberToObject(data_json, "device_id", Knee_can[equip_num].knee_ID);//����׮��ID
                cJSON_AddNumberToObject(data_json, "knee_loc", equip_num);
                break;
            }
            case EQUIP_LOGO_BACK:
            {
                printf("�ֿ�ɨ�践��\r\n");
                functioncode = SCAN_VERSION_BACK;
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "return_result", 1);
                cJSON_AddNumberToObject(data_json, "update_dev", 5);
                char ver[10] = {0};
                sprintf(ver,"%08x",UpScan_Scan[equip_num].LS);
                cJSON_AddStringToObject(data_json, "soft_version", ver);
                sprintf(ver,"%08x",UpScan_Scan[equip_num].LH);
                cJSON_AddStringToObject(data_json, "hard_version", ver);
                cJSON_AddNumberToObject(data_json, "device_id", Knee_can[equip_num].knee_ID);//����׮��ID
                cJSON_AddNumberToObject(data_json, "knee_loc", equip_num);
                break;
            }
            case EQUIP_PCRD_BACK:
            {
                printf("�ϵ縴λ��ɨ�践��\r\n");
                functioncode = SCAN_VERSION_BACK;
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "return_result", 1);
                cJSON_AddNumberToObject(data_json, "update_dev", 6);
                char ver[10] = {0};
                sprintf(ver,"%08x",UpScan_Scan[equip_num].PS);
                cJSON_AddStringToObject(data_json, "soft_version", ver);
                sprintf(ver,"%08x",UpScan_Scan[equip_num].PH);
                cJSON_AddStringToObject(data_json, "hard_version", ver);
                cJSON_AddNumberToObject(data_json, "device_id", Knee_can[equip_num].knee_ID);//����׮��ID
                cJSON_AddNumberToObject(data_json, "knee_loc", equip_num);
                break;
            }
            case PROGRAM_WRITE_BACK:
            {
                printf("�������ط���\r\n");
                functioncode = UPDATE_DOWNLOAD_BACK;
                cJSON_AddNumberToObject(data_json, "return_result", equip_num);
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                cJSON_AddNumberToObject(data_json, "site_id", Terminal.SITE_ID);
                cJSON_AddNumberToObject(data_json, "frame_number", package_count);
                break;
            }
            case UPDATE_WRITE_CAN_BACK:
            {
                functioncode = UPDATE_WRITE_BACK;
                cJSON_AddNumberToObject(data_json, "update_progress", equip_num);//��ʱequip_num��Ϊ������д����
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                if(equip_num)
                    cJSON_AddNumberToObject(data_json, "device_id", Knee_can[equip_num].knee_ID);//����׮��ID
                else
                    cJSON_AddNumberToObject(data_json, "device_id", Terminal.SITE_ID);//����վ��ID
                break;
            }
            case UPDATE_WRITE_CAN_FINISH:
            {
                functioncode = UPDATE_WRITE_BACK_FINISH;
                int return_result = 1;
                cJSON_AddNumberToObject(data_json, "return_result", return_result);
                cJSON_AddStringToObject(data_json, "site_comm_token", Terminal.TOKEN_site);
                if(equip_num)
                    cJSON_AddNumberToObject(data_json, "device_id", Knee_can[equip_num].knee_ID);//����׮��ID
                else
                    cJSON_AddNumberToObject(data_json, "device_id", Terminal.SITE_ID);//����վ��ID
                break;
            }
            default:
            {
                break;
            }
        }
        if(equip_num <= EQUIP_NUM_MAX)pthread_mutex_unlock(&Knee_can[equip_num].mt);
        /*****************************************************************/
        char *spec_data = cJSON_Print(data_json);
        cJSON_Delete(data_json);
        if(spec_data)
        {
            printf("spec_data is:%s\r\n",spec_data);
            char data_send[1024] = {0};
            Protocol_make(spec_data,data_send,1,PACKAGE_state,functioncode);
            if(write(sockfd,data_send,strlen(spec_data)+43) < 0)perror("Send");
            free(spec_data);
        }
        else printf("cJSON malloc error");
        /*****************************************************************/
    }
}
/**************************************
�������ƣ�RecvNetData()
����������������̫������
���룺
�����
������Ϣ:
***************************************/
static int RecvNetData(int sockfd)
{
    if(SITE_state == NO_LOAD)return 0;
    __u32 t;
    int recv_len;
    char data_recv[0x800] = {0};

    struct Terminal_MsgSt NetRecv_queue = {0};
    NetRecv_queue.MsgType = IPC_NOWAIT;

    struct timeval timeout={0,1000}; // ����10ms���յȴ���ʱ
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
            printf("Recv Len:%d\r\n",recv_len);
            while((recv_len -= read(sockfd, data_recv + 4 + (json_len + 35 + 4 - recv_len), recv_len))>0)
            {
                struct timeval timeout={0,1000}; // ����10ms���յȴ���ʱ
                select(0,NULL,NULL,NULL,&timeout);
                printf("recv_len is %d\r\n",recv_len);
            }
            /***********************************************************/
            int j;
            for(j=0;j<650;j++)
            {
                printf("%x ",data_recv[j]);
            }
            /***********************************************************/
            if(data_recv[41+json_len]!=0x0D || data_recv[42+json_len]!=0x0A)
            {
                printf("Tail Error!!\r\n");
                return -1;
            }
            __u16 crc = CRC16_CCITT_XMODEM(data_recv+4,json_len+35);
            if((crc>>8)!=data_recv[39+json_len] || (crc&0xFF)!=data_recv[40+json_len])
            {
                printf("CRC error!!\r\n");
                return -1;
            }
            __u64 functioncode = (data_recv[37]<<8) + data_recv[38];

            char data_recv_json[1024] = {0};
            printf("data_recv_json is :");
            for(t=0;t<json_len;t++)
            {
                data_recv_json[t] = data_recv[39+t];
                if(functioncode != UPDATE_DOWNLOAD || t<json_len-512)printf("%c",data_recv[39+t]);
            }
            cJSON *spec_data = cJSON_Parse(data_recv_json);
            SendFlag = 0;
            switch(functioncode)
            {
                case GET_CONFIG:
                {
                    File_Configuration();//��ȡ�����ļ�
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

                    File_Configuration();//��ȡ�����ļ�
                    NetRecv_queue.MsgBuff[1] = SET_CONFIG_RESULT;
                    msgsnd(CanRecv_qid, &NetRecv_queue, 2, IPC_NOWAIT);

                    if(knee_mode)
                    {
                        knees_mode = knee_mode->valuedouble;
                        if(knees_mode == 1)
                        {
                            printf("�����\r\n");
                            NetRecv_queue.MsgBuff[1] = NET;
                        }
                        else if(knees_mode == 2)
                        {
                            printf("������\r\n");
                            NetRecv_queue.MsgBuff[1] = SINGLE;
                        }
                    }
                    msgsnd(UpdataRecv_qid, &NetRecv_queue, 2, IPC_NOWAIT);
                    break;
                }
                case EXIT_VIRTUAL_SERVER:
                {
                    printf("�˳����������\r\n");
                    isRunning = PROGRAM_ACTUAL_SERVER;  //��ת����ʵ������
                    break;
                }
                case ENTER_BOOTLOADER:
                {
                    cJSON *enter_bld = cJSON_GetObjectItem(spec_data,"enter_bld");
                    if(enter_bld && enter_bld->valueint == 0)
                    {
                        printf("�˳�BOOTLOADER ����APP���������������\r\n");
                        NetRecv_queue.MsgBuff[1] = BOOTLOAD_OUT;
                        msgsnd(CanRecv_qid, &NetRecv_queue, 2, IPC_NOWAIT);
                    }
                    else if(enter_bld && enter_bld->valueint == 1)
                    {
                        printf("����BOOTLOADER\r\n");
                        NetRecv_queue.MsgBuff[1] = BOOTLOAD_IN;
                        msgsnd(CanRecv_qid, &NetRecv_queue, 2, IPC_NOWAIT);
                    }
                    else printf("�Ƿ�ָ��\r\n");
                    break;
                }
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
                case UPDATE_DOWNLOAD:
                {
                    cJSON *program_size = cJSON_GetObjectItem(spec_data,"program_size");
                    cJSON *frame_number = cJSON_GetObjectItem(spec_data,"frame_number");
                    if(program_size && frame_number)
                    {
                        UpData_data.program_size = (__u64)program_size->valuedouble;
                        if(frame_number->valueint == 0)
                        {
                            if(!remove(PROGRAM_PATH))printf("updateFileɾ���ɹ�\n");
                            package_count = -1;
                        }
                        if(frame_number->valueint == package_count + 1)
                        {
                            int packet_total = program_size->valueint/512 + (program_size->valueint%512?1:0);//������
                            printf("packet_total:%d\r\n",packet_total);

                            if(frame_number->valueint >= packet_total)printf("�����������\r\n");
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
                            printf("����������\r\n");
                            NetRecv_queue.MsgBuff[0] = 1;
                            NetRecv_queue.MsgBuff[1] = PROGRAM_WRITE_BACK;
                            msgsnd(CanRecv_qid, &NetRecv_queue, 2, IPC_NOWAIT);
                        }
                    }
                    else printf("��������ȫ\r\n");
                    break;
                }
                case UPDATE_WRITE:
                {
                    printf("���� ");
                    cJSON *device_id = cJSON_GetObjectItem(spec_data,"device_id");
                    cJSON *update_dev = cJSON_GetObjectItem(spec_data,"update_dev");
                    cJSON *knee_loc = cJSON_GetObjectItem(spec_data,"knee_loc");
                    if(device_id && update_dev && knee_loc)
                    {
                        UpData_data.equip_name = update_dev->valueint;
                        switch (update_dev->valueint)
                        {
                        case 1:
                        {
                            printf("�ն˳���\r\n");
                            ClearFlagAPP();//���APPFLAG
                            if(chmod(PROGRAM_PATH,S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH)<0)perror("chmodʧ��\r\n");
                            if(pthread_create(&pt_tid,NULL,ProgramTransfer_thread,NULL))
                            {
                                perror("Create UPDATE thread error!");
                                return;
                            }
                            break;
                        }
                        case 2:
                        {
                            printf("���س���\r\n");
                            NetRecv_queue.MsgBuff[0] = knee_loc->valueint;
                            NetRecv_queue.MsgBuff[1] = BOOT_IN;
                            break;
                        }
                        case 3:
                        {
                            printf("�Կس���\r\n");
                            NetRecv_queue.MsgBuff[0] = knee_loc->valueint;
                            NetRecv_queue.MsgBuff[1] = BOOT_IN;
                            break;
                        }
                        case 4:
                        {
                            printf("�ز�����\r\n");
                            NetRecv_queue.MsgBuff[0] = knee_loc->valueint;
                            NetRecv_queue.MsgBuff[1] = BOOT_IN;
                            break;
                        }
                        case 5:
                        {
                            printf("�ֿ�LOGO\r\n");
                            NetRecv_queue.MsgBuff[0] = knee_loc->valueint;
                            NetRecv_queue.MsgBuff[1] = BOOT_IN;
                            break;
                        }
                        case 6:
                        {
                            printf("�ϵ縴λ��\r\n");
                            NetRecv_queue.MsgBuff[0] = knee_loc->valueint;
                            NetRecv_queue.MsgBuff[1] = BOOT_IN;
                            break;
                        }
                        default:
                            printf("�豸���ʹ���\r\n");
                            break;
                        }
                    }
                    else printf("��������ȫ\r\n");
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

void TCP_KPD(int sockfd)
{
    serve(sockfd);
    NetSendData(sockfd);
    RecvNetData(sockfd);
}
