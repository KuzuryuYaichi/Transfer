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
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/msg.h>
#include <fcntl.h>
#include "UDP.h"
#include "TCP.h"
#include "can.h"
#include "Terminal.h"
#include "CJSON.h"
#include "USART.h"
#include "Base64.h"
#include "CommonFunc.h"

extern struct Terminal_Info Terminal;
extern struct Knee_struct Knee_can[EQUIP_NUM_MAX + 1];

const char *TER_PATH = "/update/t2";
const char *MCU_PATH = "/update/mcu";
const char *DIS_PATH = "/update/dis";
const char *PROGRAM_PATH = "/update/program";

/**************************************
函数名称：valueToHexCh
功能描述：
输入：
输出：
创建信息：@author GYQ
***************************************/
char valueToHexCh(const int value)
{
    char result = '\0';
    if(value >= 0 && value <= 9)
    {
        result = (char)(value+48);  //48为ASCII编码的‘0’字符编码值
    }
    else if(value >= 10 && value <= 15)
    {
        result = (char)(value-10+65);   //减去10则找出其在16进制的偏移量，65为ASCII的‘A’的字符编码值
    }
    else
    {;}
    return result;
}
/**************************************
函数名称：StrToHex
功能描述：
输入：
输出：
创建信息：@author GYQ
***************************************/
int StrToHex(char *ch,__u32 len,char *hex)
{
    __u32 t = 0;
    int high,low;
    int tmp = 0;
    for(t=0;t<len;t++)
    {
        tmp = (int)*ch;
        high = tmp >> 4;
        low = tmp & 15;
        *hex++ = valueToHexCh(high);  //先写高字节
        *hex++ = valueToHexCh(low);   //其次写低字节
        ch++;
    }
    *hex = '\0';
    return 0;
}
/**************************************
函数名称：Hex_Dec
功能描述：16进制转10进制(不等值转换)
输入：
输出：
创建信息：@author GYQ
***************************************/
__u64 Hex_Dec(char *HexData,int HexData_len)
{
    int temp;
    __u64 DecData = 0;
    for(temp=0;temp<HexData_len;temp++)
    {
        __u64 Data = ((((*(HexData+temp)>>4)&0x0F)*10) + (*(HexData+temp)&0x0F));
        int temp2 = HexData_len-1-temp;
        for(;temp2>0;temp2--)
        {
            Data *= 100;
        }
        DecData += Data;
    }
    return DecData;
}
/**************************************
函数名称：Hex_to_Dec
功能描述：4字节 16进制转10进制(等值转换)
输入：
输出：
创建信息：@author GYQ
***************************************/
__u64 Hex_to_Dec(__u8 *s)
{
    int i,t;
    __u64 sum = 0;
    for(i=0;i<4;i++)
    {
        __u64 dec_temp = s[i];
        for(t=3;t>i;t--)
        {
            dec_temp = dec_temp*256;
        }
        sum += dec_temp;
        dec_temp = 0;
    }
    return sum;
}
/**************************************
函数名称：Dec_to_Hex
功能描述：10进制转16进制(等值转换)
输入：
输出：
创建信息：@author GYQ
***************************************/
void Dec_to_Hex(__u64 Dec,__u8 *Hex)
{
    int j,k;
    int count = 0;
    int Remainder;
    while(Dec)
    {
        Remainder = Dec%16;
        if((Remainder>=10) && (Remainder<=15))
        {
            *(Hex+count) = 'A'+Remainder-10;
            count++;
        }
        else
        {
            if(Remainder == 0)
            {
                *(Hex+count) = '0';
                count++;
            }
            else
            {
                *(Hex+count) = '0'+Remainder;
                count++;
            }
        }
        Dec /= 16;
    }
    if(count>0)
    {
        for(j=0,k=count-1;j<count/2;j++,k--)
        {
            __u8 temp;
            temp = *(Hex+j);
            *(Hex+j) = *(Hex+k);
            *(Hex+k) = temp;
        }
        *(Hex+count) = '\0';
    }
}
/**************************************
函数名称：ID_to_Token
功能描述：ID转Token
输入： head-0x02,zone-0x44,id-车辆id,id_len-车辆id长度
输出： token-生成的TOKEN字符串
创建信息：@author GYQ
***************************************/
char* ID_to_Token(__u8 *id,__u8 id_len,char *token)
{
    __u8 count;
    __u8 LongId[20] = {'0'};
    __u8 ID_Hex[64] = {0};
    __u64 ID_Dec = Hex_Dec(id,id_len);    //ID数组转换为10进制整型
    Dec_to_Hex(ID_Dec,ID_Hex);	//10进制ID转换为16进制
    __u8 ID_Hex_len = strlen(ID_Hex);
    memset(LongId,'0',20);

    LongId[0] = '0';
    LongId[1] = '2';
    LongId[2] = '4';
    LongId[3] = '4';

    for(count=0;count<ID_Hex_len;count++)
    {
        LongId[19-count] = *(ID_Hex+ID_Hex_len-1-count);
    }
    base64_encode(LongId,token,20);
    return token;
}
/**************************************
函数名称：Hex2Dec
功能描述：10进制转16进制(4位转换)
输入：
输出：
创建信息: @author LYH
***************************************/
int Dec2Hex(__u64 Dec, __u8 *Hex)
{
    if (Dec)
    {
        int i = Dec2Hex(Dec / 100, Hex);
        Hex[i] = Dec % 100 / 10 * 16 + Dec % 100 % 10;
        return i + 1;
    }
    return 0;
}
/**************************************
函数名称：Hex2Dec
功能描述：16进制转10进制(4位转换)
输入：
输出：
创建信息: @author LYH
***************************************/
__u8 Hex2Dec(__u8 HexData)
{
    return HexData?(Hex2Dec(HexData/10)<<4)+HexData%10:0;
}
/***************************************
函数名称：num2str
功能描述：数字转字符串
输入：
输出：
创建信息: @author LYH
***************************************/
char* num2str(char *at_send,int len)
{
    if(len)sprintf(at_send,"%s%c",num2str(at_send,len/10),'0'+len%10);
    return at_send;
}
/***************************************
函数名称：findKneeNum
功能描述：轮询查找桩号
输入：
输出：
创建信息: @author LYH
***************************************/
__u8 findKneeNum(__u64 knee_SN)
{
    __u8 t;
    for(t=0;t<=EQUIP_NUM_MAX;t++)
    {
        pthread_mutex_lock(&Knee_can[t].mt);
        if(Knee_can[t].knee_ID == knee_SN)
        {
            pthread_mutex_unlock(&Knee_can[t].mt);
            return t;
        }
        pthread_mutex_unlock(&Knee_can[t].mt);
    }
    return t;
}
/*****************************************************
函数名称：site_id_reset
功能描述：重新写入站点配置信息
输入：
输出：
创建信息: @author LYH
 *****************************************************/
void site_id_reset(void* value,__u8 type)
{
    const char *path = NULL;
    if(type == SUCC_COUNT_WRITE || type == FAIL_COUNT_WRITE || type == RESET_COUNT_WRITE)path = "/connect_record";
    else path = "/site_info";

    char *szBuf = (char*)malloc(1000);
    memset(szBuf,0,1000);
    int fd;
    if((fd = open(path,O_CREAT | O_RDWR,0600)) > 0)
    {
        while(flock(fd,LOCK_EX|LOCK_NB));
        lseek(fd,0,SEEK_SET);
        if(read(fd,szBuf,1000) < 0)perror("read");
        while(flock(fd,LOCK_UN));
        close(fd);
    }
    else perror("open");

    cJSON *spec_data = cJSON_Parse(szBuf);//spec_data为接收到的JSON指令
    free(szBuf);
    if(!spec_data)spec_data = cJSON_CreateObject();

    switch(type)
    {
        case BACKSTAGE_SELECT:
        {
            cJSON *backstage_sel = cJSON_GetObjectItem(spec_data,"backstage_sel");     //站点ID
            if(backstage_sel){
                cJSON_ReplaceItemInObject(spec_data,"backstage_sel",cJSON_CreateNumber(*(int*)value));
            }else{
                cJSON_AddNumberToObject(spec_data,"backstage_sel", *(int*)value);
            }
            break;
        }
        case WIFI_PASSWORD_WRITE:
        {
            cJSON *password = cJSON_GetObjectItem(spec_data,"wifi_password");     //站点ID
            if(password){
                cJSON_ReplaceItemInObject(spec_data,"wifi_password",cJSON_CreateString((const char*)value));
            }else{
                cJSON_AddStringToObject(spec_data, "wifi_password", value);
            }
            break;
        }
        case WIFI_NAME_WRITE:
        {
            cJSON *name = cJSON_GetObjectItem(spec_data,"wifi_name");     //站点ID
            if(name){
                cJSON_ReplaceItemInObject(spec_data,"wifi_name",cJSON_CreateString((const char*)value));
            }else{
                cJSON_AddStringToObject(spec_data,"wifi_name", value);
            }
            break;
        }
        case SERVER_IP_WRITE:
        {
            cJSON *ip = cJSON_GetObjectItem(spec_data,"server_ip");
            if(ip){
                cJSON_ReplaceItemInObject(spec_data,"server_ip",cJSON_CreateString((const char*)value));
            }else{
                cJSON_AddStringToObject(spec_data,"server_ip", value);
            }
            break;
        }
        case SERVER_PORT_WRITE:
        {
            cJSON *port = cJSON_GetObjectItem(spec_data,"server_port");
            if(port){
                cJSON_ReplaceItemInObject(spec_data,"server_port",cJSON_CreateNumber(*(int*)value));
            }else{
                cJSON_AddNumberToObject(spec_data,"server_port", *(int*)value);
            }
            break;
        }
        case SITE_ID_WRITE:
        {
            cJSON *id = cJSON_GetObjectItem(spec_data,"site_id");
            if(id){
                cJSON_ReplaceItemInObject(spec_data,"site_id",cJSON_CreateNumber(*(double*)value));
            }else{
                cJSON_AddNumberToObject(spec_data, "site_id", *(double*)value);
            }
            break;
        }
        case SITE_PASSWORD_WRITE:
        {
            cJSON *password = cJSON_GetObjectItem(spec_data,"site_password");     //站点密码
            if(password){
                cJSON_ReplaceItemInObject(spec_data,"site_password",cJSON_CreateString((const char*)value));
            }else{
                cJSON_AddStringToObject(spec_data, "site_password", value);
            }
            break;
        }
        case SITE_DNS_WRITE:
        {
            cJSON *dns = cJSON_GetObjectItem(spec_data,"server_dns");     //站点dns
            if(dns){
                cJSON_ReplaceItemInObject(spec_data,"server_dns",cJSON_CreateString((const char*)value));
            }else{
                cJSON_AddStringToObject(spec_data, "server_dns", value);
            }
            break;
        }
        case SUCC_COUNT_WRITE:
        {
            cJSON *succ_login = cJSON_GetObjectItem(spec_data,"ConnectCount");
            if(succ_login){
                cJSON_ReplaceItemInObject(spec_data,"ConnectCount",cJSON_CreateNumber(*(double*)value));
            }else{
                cJSON_AddNumberToObject(spec_data, "ConnectCount", *(double*)value);
            }
            break;
        }
        case FAIL_COUNT_WRITE:
        {
            cJSON *fail_login = cJSON_GetObjectItem(spec_data,"DisConnectCount");
            if(fail_login){
                cJSON_ReplaceItemInObject(spec_data,"DisConnectCount",cJSON_CreateNumber(*(double*)value));
            }else{
                cJSON_AddNumberToObject(spec_data, "DisConnectCount", *(double*)value);
            }
            break;
        }
        case RESET_COUNT_WRITE:
        {
            cJSON *reset_count = cJSON_GetObjectItem(spec_data,"ResetCount");
            if(reset_count){
                cJSON_ReplaceItemInObject(spec_data,"ResetCount",cJSON_CreateNumber(*(double*)value));
            }else{
                cJSON_AddNumberToObject(spec_data, "ResetCount", *(double*)value);
            }
            break;
        }
        default:break;
    }

    szBuf = cJSON_Print(spec_data);
    cJSON_Delete(spec_data);

    if((fd = open(path,O_CREAT | O_RDWR,0600)) > 0)
    {
        while(flock(fd,LOCK_EX|LOCK_NB));
        lseek(fd,0,SEEK_SET);
        if(write(fd,szBuf,strlen(szBuf))!=strlen(szBuf))perror("write");
        else printf("写入成功!\r\n");
        while(flock(fd,LOCK_UN));
        close(fd);
    }
    else perror("open");

    if(szBuf)free(szBuf);
}
/**************************************
函数名称：   ResetRecord
功能描述：   记录重启次数
输入：
输出：
创建信息：
***************************************/
int ResetRecord()
{
    char szBuf[128] = {0};
    int fd;
    if((fd = open("/connect_record",O_CREAT | O_RDWR,0600)) > 0)
    {
        while(flock(fd,LOCK_EX|LOCK_NB));
        lseek(fd,0,SEEK_SET);
        if(read(fd,szBuf,sizeof(szBuf)) == -1)perror("read");
        while(flock(fd,LOCK_UN));
        close(fd);
    }
    else perror("open");
    printf("read bytes:%s\r\n",szBuf);

    cJSON *spec_data = cJSON_Parse(szBuf);
    if(!spec_data)spec_data = cJSON_CreateObject();

    cJSON *ResetCountPtr = cJSON_GetObjectItem(spec_data,"ResetCount");//站点ID
    if(ResetCountPtr)
    {
        cJSON_ReplaceItemInObject(spec_data,"ResetCount",cJSON_CreateNumber(ResetCountPtr->valueint+1));
        printf("重连服务器成功次数:%d\r\n",ResetCountPtr->valueint+1);
        Terminal.ResetCount = ResetCountPtr->valueint+1;
    }
    else
    {
        cJSON_AddNumberToObject(spec_data,"ResetCount",0);
    }

    char *buf = cJSON_Print(spec_data);
    cJSON_Delete(spec_data);
    // 回写缓存文件
    if((fd = open("/connect_record",O_CREAT | O_RDWR,0600)) > 0)
    {
        while(flock(fd,LOCK_EX|LOCK_NB));
        lseek(fd,0,SEEK_SET);
        if(write(fd,buf,strlen(buf)) != strlen(buf))perror("write");
        while(flock(fd,LOCK_UN));
        close(fd);
    }
    else perror("open");
    free(buf);
}
/**************************************
函数名称：ConnectRecord
功能描述：从协议包中提取JSON格式数据
输入：
输出：
创建信息：
***************************************/
int ConnectRecord()
{
    char szBuf[128] = {0};
    int fd;
    if((fd = open("/connect_record",O_CREAT | O_RDWR,0600)) > 0)
    {
        while(flock(fd,LOCK_EX|LOCK_NB));
        lseek(fd,0,SEEK_SET);
        if(read(fd,szBuf,sizeof(szBuf)) == -1)perror("read");
        while(flock(fd,LOCK_UN));
        close(fd);
    }
    else perror("open");
    printf("read bytes:%s\r\n",szBuf);

    cJSON *spec_data = cJSON_Parse(szBuf);//spec_data为读取到的JSON文件
    if(!spec_data)spec_data = cJSON_CreateObject();

    cJSON *ConnectCountPtr = cJSON_GetObjectItem(spec_data,"ConnectCount");//站点ID
    if(ConnectCountPtr)
    {
        cJSON_ReplaceItemInObject(spec_data,"ConnectCount",cJSON_CreateNumber(ConnectCountPtr->valueint+1));
        printf("重连服务器成功次数:%d\r\n",ConnectCountPtr->valueint+1);
        Terminal.ConnectCount = ConnectCountPtr->valueint+1;
    }
    else
    {
        cJSON_AddNumberToObject(spec_data, "ConnectCount", 0);
    }

    char *buf = cJSON_Print(spec_data);
    cJSON_Delete(spec_data);
    // 回写缓存文件
    if((fd = open("/connect_record",O_CREAT | O_RDWR,0600)) > 0)
    {
        while(flock(fd,LOCK_EX|LOCK_NB));
        lseek(fd,0,SEEK_SET);
        if(write(fd,buf,strlen(buf)) != strlen(buf))perror("write");
        while(flock(fd,LOCK_UN));
        close(fd);
    }
    else perror("open");
    free(buf);
}
/**************************************
函数名称：   DisConnectRecord
功能描述：   断线记录
输入：
输出：
创建信息：
***************************************/
int DisConnectRecord()
{
    char szBuf[128] = {0};
    int fd;
    if((fd = open("/connect_record",O_CREAT | O_RDWR,0600)) > 0)
    {
        while(flock(fd,LOCK_EX|LOCK_NB));
        lseek(fd,0,SEEK_SET);
        if(read(fd,szBuf,sizeof(szBuf)) == -1)perror("read");
        while(flock(fd,LOCK_UN));
        close(fd);
    }
    else perror("open");
    printf("read bytes:%s\r\n",szBuf);

    cJSON *spec_data = cJSON_Parse(szBuf);//spec_data为读取到的JSON文件
    if(!spec_data)spec_data = cJSON_CreateObject();

    cJSON *DisconnectCountPtr = cJSON_GetObjectItem(spec_data,"DisconnectCount");//站点ID
    if(DisconnectCountPtr)
    {
        cJSON_ReplaceItemInObject(spec_data,"DisconnectCount",cJSON_CreateNumber(DisconnectCountPtr->valueint+6));
        printf("心跳无返回次数:%d\r\n",DisconnectCountPtr->valueint+6);
        Terminal.DisConnectCount = DisconnectCountPtr->valueint+6;
    }
    else
    {
        cJSON_AddNumberToObject(spec_data, "DisconnectCount", 0);
    }

    char *buf = cJSON_Print(spec_data);
    cJSON_Delete(spec_data);
    // 回写缓存文件
    if((fd = open("/connect_record",O_CREAT | O_RDWR,0600)) > 0)
    {
        while(flock(fd,LOCK_EX|LOCK_NB));
        lseek(fd,0,SEEK_SET);
        if(write(fd,buf,strlen(buf)) != strlen(buf))perror("write");
        while(flock(fd,LOCK_UN));
        close(fd);
    }
    else perror("open");
    free(buf);
}

/**************************************
函数名称：ClearFlagAPP()
功能描述：擦除APPFLAG标志位
输入：
输出：
创建信息:
***************************************/
void ClearFlagAPP()
{
    char *path = "/version";
    char szBuf[128] = {0};
    int fd;
    if((fd = open(path,O_CREAT | O_RDWR,0600)) > 0)
    {
        while(flock(fd,LOCK_EX|LOCK_NB));
        lseek(fd,0,SEEK_SET);
        if(read(fd,szBuf,sizeof(szBuf)) == -1)perror("read");
        while(flock(fd,LOCK_UN));
        close(fd);
    }
    else perror("open");

    cJSON *spec_data = cJSON_Parse(szBuf);
    if(!spec_data)spec_data = cJSON_CreateObject();

    cJSON *appflag = cJSON_GetObjectItem(spec_data,"appflag");
    if(appflag)
        cJSON_ReplaceItemInObject(spec_data,"appflag",cJSON_CreateNumber(0));
    else
        cJSON_AddNumberToObject(spec_data, "appflag", 0);

    char *buf = cJSON_Print(spec_data);
    cJSON_Delete(spec_data);
    // 回写缓存文件
    if((fd = open(path,O_CREAT | O_RDWR,0600)) > 0)
    {
        while(flock(fd,LOCK_EX|LOCK_NB));
        lseek(fd,0,SEEK_SET);
        if(write(fd,buf,strlen(buf)) == -1)perror("write");
        while(flock(fd,LOCK_UN));
        close(fd);
    }
    else perror("open");
    free(buf);
    printf("清除AppFlag\r\n");
}
/**************************************
函数名称：WriteVersionInfo
功能描述：写入版本信息
输入：
输出：
创建信息:
***************************************/
void WriteVersionInfo(char *software,char *hardware)
{
    if(strlen(software) > 16 || strlen(hardware) > 16){
        perror("版本号过长");
        return;
    }
    char *path = "/version";
    char szBuf[128] = {0};
    int fd;
    if((fd = open(path,O_CREAT | O_RDWR,0600)) > 0)
    {
        while(flock(fd,LOCK_EX|LOCK_NB));
        lseek(fd,0,SEEK_SET);
        if(read(fd,szBuf,sizeof(szBuf)) == -1)perror("read");
        while(flock(fd,LOCK_UN));
        close(fd);
    }
    else perror("open");

    cJSON *spec_data = cJSON_Parse(szBuf);
    if(!spec_data)spec_data = cJSON_CreateObject();

    cJSON *soft_version = cJSON_GetObjectItem(spec_data,"soft_version");
    if(soft_version)
        cJSON_ReplaceItemInObject(spec_data,"soft_version",cJSON_CreateString(software));
    else
        cJSON_AddStringToObject(spec_data, "soft_version", software);

    cJSON *hard_version = cJSON_GetObjectItem(spec_data,"hard_version");
    if(hard_version)
        cJSON_ReplaceItemInObject(spec_data,"hard_version",cJSON_CreateString(hardware));
    else
        cJSON_AddStringToObject(spec_data, "hard_version", hardware);

    char *buf = cJSON_Print(spec_data);
    cJSON_Delete(spec_data);
    // 回写缓存文件
    if((fd = open(path,O_CREAT | O_RDWR,0600)) > 0)
    {
        while(flock(fd,LOCK_EX|LOCK_NB));
        lseek(fd,0,SEEK_SET);
        if(write(fd,buf,strlen(buf)) == -1)perror("write");
        while(flock(fd,LOCK_UN));
        close(fd);
    }
    else perror("open");
    free(buf);
    printf("重新写入版本号完成\r\n");
}
/**************************************
函数名称：ProgramTransfer()
功能描述：程序搬运
输入：
输出：
创建信息:
***************************************/
void ProgramTransfer(const char *dest,const char *src)
{
    if(remove(dest))perror("删除t2失败\r\n");
    else
    {
        char szBuf[4096] = {0};
        int dest_fd,src_fd;
        if((src_fd = open(src,O_CREAT | O_RDWR,0600)) > 0)
        {
            if(read(src_fd,szBuf,64) == 64)
            {
                printf("\r\nProgramType:%s\r\nSoftwareVersion:%s\r\nHardwareVersion:%s\r\n",szBuf,szBuf+16,szBuf+32);
                WriteVersionInfo(szBuf+16,szBuf+32);//写入新的版本号
                if((dest_fd = open(dest,O_CREAT | O_RDWR | O_APPEND,0600)) > 0)
                {
                    struct Terminal_MsgSt ter = {0};
                    ter.MsgType = IPC_NOWAIT;
                    int filesize = lseek(src_fd,0,SEEK_END);
                    lseek(src_fd,64,SEEK_SET);
                    int total = 64;
                    int readlen;
                    do
                    {
                        while(flock(src_fd,LOCK_EX|LOCK_NB));
                        readlen = read(src_fd,szBuf,sizeof(szBuf));
                        while(flock(src_fd,LOCK_UN));
                        while(flock(dest_fd,LOCK_EX|LOCK_NB));
                        if(write(dest_fd,szBuf,readlen) != readlen)perror("Write Error");
                        while(flock(dest_fd,LOCK_UN));

                        ter.MsgBuff[1] = UPDATE_WRITE_CAN_BACK;
                        ter.MsgBuff[0] = 100*(total += readlen)/filesize;
                        msgsnd(CanRecv_qid, &ter, 2, IPC_NOWAIT);

                        struct timeval time = {0,100000};
                        select(0,NULL,NULL,NULL,&time);
                    }while(readlen == sizeof(szBuf));

                    if(chmod(dest,S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH)<0)perror("chmod失败\r\n");
                    printf("程序转移完毕\r\n");

                    ter.MsgBuff[1] = UPDATE_WRITE_CAN_FINISH;
                    ter.MsgBuff[0] = 1;
                    msgsnd(CanRecv_qid, &ter, 2, IPC_NOWAIT);

                    close(dest_fd);
                }
                else perror("open dests");
            }
            else perror("Read Program Information Error\r\n");
            close(src_fd);
        }
        else perror("open src");
    }
}
/**************************************
函数名称：ProgramTransfer_thread
功能描述：程序搬运线程
输入：
输出：
创建信息:
***************************************/
void* ProgramTransfer_thread(void *arg)
{
    pthread_detach(pthread_self());
    ProgramTransfer("/t2",PROGRAM_PATH);//程序转移
}
/**************************************
函数名称：Terminal_Updata
功能描述：数据终端升级写入
输入：
输出：
创建信息：@author LYH
***************************************/
int Terminal_Updata(char *recv_order,int data_len,const char *path)
{
    int fd_Ter;
    int up_state = 0;
    if((fd_Ter = open(path,O_CREAT | O_RDWR | O_APPEND,0600)) < 0)
    {
        perror("open");
    }
    while(flock(fd_Ter,LOCK_EX|LOCK_NB));
    if(write(fd_Ter,recv_order,data_len)!=data_len)
    {
        up_state = 0;
        perror("write");
    }
    else
    {
        up_state = 1;
    }
    while(flock(fd_Ter,LOCK_UN));
    close(fd_Ter);
    return up_state;
}
