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
#include "USART.h"
#include "Base64.h"
#include "CommonFunc.h"

extern struct Terminal_Info Terminal;
extern struct Knee_struct Knee_can[EQUIP_NUM_MAX+1];

/**************************************
�������ƣ�valueToHexCh
����������
���룺
�����
������Ϣ��@author GYQ
***************************************/
char valueToHexCh(const int value)
{
    char result = '\0';
    if(value >= 0 && value <= 9)
    {
        result = (char)(value+48);  //48ΪASCII����ġ�0���ַ�����ֵ
    }
    else if(value >= 10 && value <= 15)
    {
        result = (char)(value-10+65);   //��ȥ10���ҳ�����16���Ƶ�ƫ������65ΪASCII�ġ�A�����ַ�����ֵ
    }
    else
    {;}
    return result;
}
/**************************************
�������ƣ�StrToHex
����������
���룺
�����
������Ϣ��@author GYQ
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
        *hex++ = valueToHexCh(high);  //��д���ֽ�
        *hex++ = valueToHexCh(low);   //���д���ֽ�
        ch++;
    }
    *hex = '\0';
    return 0;
}
/**************************************
�������ƣ�Hex_Dec
����������16����ת10����(����ֵת��)
���룺
�����
������Ϣ��@author GYQ
***************************************/
__u64 Hex_Dec(char *HexData,int HexData_len)
{
    int temp;
    __u64 DecData = 0;
    for(temp=0;temp<HexData_len;temp++)
    {
        __u64 Data = ((HexData[temp]>>4)&0x0F)*10 + (HexData[temp]&0x0F);
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
�������ƣ�Hex_to_Dec
����������4�ֽ� 16����ת10����(��ֵת��)
���룺
�����
������Ϣ��@author GYQ
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
�������ƣ�Dec_to_Hex
����������10����ת16����(��ֵת��)
���룺
�����
������Ϣ��@author GYQ
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
�������ƣ�ID_to_Token
����������IDתToken
���룺 head-0x02,zone-0x44,id-����id,id_len-����id����
����� token-���ɵ�TOKEN�ַ���
������Ϣ��@author GYQ
***************************************/
char* ID_to_Token(__u8 *id,__u8 id_len,char *token)
{
    __u8 count;
    __u8 LongId[20] = {'0'};
    __u8 ID_Hex[64] = {0};
    __u64 ID_Dec = Hex_Dec(id,id_len);    //ID����ת��Ϊ10��������
    Dec_to_Hex(ID_Dec,ID_Hex);	//10����IDת��Ϊ16����
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
�������ƣ�Hex2Dec
����������10����ת16����(4λת��)
���룺
�����
������Ϣ: @author LYH
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
�������ƣ�Hex2Dec
����������16����ת10����(4λת��)
���룺
�����
������Ϣ: @author LYH
***************************************/
__u8 Hex2Dec(__u8 HexData)
{
    return HexData?(Hex2Dec(HexData/10)<<4)+HexData%10:0;
}
/***************************************
�������ƣ�num2str
��������������ת�ַ���
���룺
�����
������Ϣ: @author LYH
***************************************/
char* num2str(char *at_send,int len)
{
    if(len)sprintf(at_send,"%s%c",num2str(at_send,len/10),'0'+len%10);
    return at_send;
}
/***************************************
�������ƣ�findKneeNum
������������ѯ����׮��
���룺
�����
������Ϣ: @author LYH
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
�������ƣ�site_id_reset
��������������д��վ��������Ϣ
���룺
�����
������Ϣ: @author LYH
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

    cJSON *spec_data = cJSON_Parse(szBuf);//spec_dataΪ���յ���JSONָ��
    free(szBuf);
    if(!spec_data)spec_data = cJSON_CreateObject();

    switch(type)
    {
        case BACKSTAGE_SELECT:
        {
            cJSON *backstage_sel = cJSON_GetObjectItem(spec_data,"backstage_sel");     //վ��ID
            if(backstage_sel){
                cJSON_ReplaceItemInObject(spec_data,"backstage_sel",cJSON_CreateNumber(*(int*)value));
            }else{
                cJSON_AddNumberToObject(spec_data,"backstage_sel", *(int*)value);
            }
            break;
        }
        case WIFI_PASSWORD_WRITE:
        {
            cJSON *password = cJSON_GetObjectItem(spec_data,"wifi_password");     //վ��ID
            if(password){
                cJSON_ReplaceItemInObject(spec_data,"wifi_password",cJSON_CreateString((const char*)value));
            }else{
                cJSON_AddStringToObject(spec_data, "wifi_password", value);
            }
            break;
        }
        case WIFI_NAME_WRITE:
        {
            cJSON *name = cJSON_GetObjectItem(spec_data,"wifi_name");     //վ��ID
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
            cJSON *password = cJSON_GetObjectItem(spec_data,"site_password");     //վ������
            if(password){
                cJSON_ReplaceItemInObject(spec_data,"site_password",cJSON_CreateString((const char*)value));
            }else{
                cJSON_AddStringToObject(spec_data, "site_password", value);
            }
            break;
        }
        case SITE_DNS_WRITE:
        {
            cJSON *dns = cJSON_GetObjectItem(spec_data,"server_dns");     //վ��dns
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
        case CUSTOMER_ID_WRITE:
        {
            cJSON *customer_id = cJSON_GetObjectItem(spec_data,"customer_id");
            if(customer_id){
                cJSON_ReplaceItemInObject(spec_data,"customer_id",cJSON_CreateNumber(*(int*)value));
            }else{
                cJSON_AddNumberToObject(spec_data,"customer_id", *(int*)value);
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
        else printf("д��ɹ�!\r\n");
        while(flock(fd,LOCK_UN));
        close(fd);
    }
    else perror("open");

    if(szBuf)free(szBuf);
}
/**************************************
�������ƣ�   ResetRecord
����������   ��¼��������
���룺
�����
������Ϣ��
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

    cJSON *spec_data = cJSON_Parse(szBuf);
    if(!spec_data)spec_data = cJSON_CreateObject();

    cJSON *ResetCountPtr = cJSON_GetObjectItem(spec_data,"ResetCount");//վ��ID
    if(ResetCountPtr)
    {
        cJSON_ReplaceItemInObject(spec_data,"ResetCount",cJSON_CreateNumber(ResetCountPtr->valueint+1));
        printf("�����������ɹ�����:%d\r\n",ResetCountPtr->valueint+1);
        Terminal.ResetCount = ResetCountPtr->valueint+1;
    }
    else
    {
        cJSON_AddNumberToObject(spec_data,"ResetCount",0);
    }

    char *buf = cJSON_Print(spec_data);
    cJSON_Delete(spec_data);
    // ��д�����ļ�
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
�������ƣ�ConnectRecord
������������Э�������ȡJSON��ʽ����
���룺
�����
������Ϣ��
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

    cJSON *spec_data = cJSON_Parse(szBuf);//spec_dataΪ��ȡ����JSON�ļ�
    if(!spec_data)spec_data = cJSON_CreateObject();

    cJSON *ConnectCountPtr = cJSON_GetObjectItem(spec_data,"ConnectCount");//վ��ID
    if(ConnectCountPtr)
    {
        cJSON_ReplaceItemInObject(spec_data,"ConnectCount",cJSON_CreateNumber(ConnectCountPtr->valueint+1));
        printf("�����������ɹ�����:%d\r\n",ConnectCountPtr->valueint+1);
        Terminal.ConnectCount = ConnectCountPtr->valueint+1;
    }
    else
    {
        cJSON_AddNumberToObject(spec_data, "ConnectCount", 0);
    }

    char *buf = cJSON_Print(spec_data);
    cJSON_Delete(spec_data);
    // ��д�����ļ�
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
�������ƣ�   DisConnectRecord
����������   ���߼�¼
���룺
�����
������Ϣ��
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

    cJSON *spec_data = cJSON_Parse(szBuf);//spec_dataΪ��ȡ����JSON�ļ�
    if(!spec_data)spec_data = cJSON_CreateObject();

    cJSON *DisconnectCountPtr = cJSON_GetObjectItem(spec_data,"DisconnectCount");//վ��ID
    if(DisconnectCountPtr)
    {
        cJSON_ReplaceItemInObject(spec_data,"DisconnectCount",cJSON_CreateNumber(DisconnectCountPtr->valueint+1));
        printf("�����޷��ش���:%d\r\n",DisconnectCountPtr->valueint+1);
        Terminal.DisConnectCount = DisconnectCountPtr->valueint+1;
    }
    else
    {
        cJSON_AddNumberToObject(spec_data, "DisconnectCount", 0);
    }

    char *buf = cJSON_Print(spec_data);
    cJSON_Delete(spec_data);
    // ��д�����ļ�
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

//д�ϵ縴λ����
int ReadFlags()
{
    cJSON *spec_data = NULL;
    char szBuf[256] = {0};
    int fd;
    if((fd = open("/flag_info",O_CREAT | O_RDWR,0600)) > 0)
    {
        while(flock(fd,LOCK_EX|LOCK_NB));
        lseek(fd,0,SEEK_SET);
        if(read(fd,szBuf,sizeof(szBuf)) == -1)perror("read");
        while(flock(fd,LOCK_UN));
        close(fd);

        spec_data = cJSON_Parse(szBuf);//spec_dataΪ��ȡ����JSON�ļ�
        if(spec_data)
        {
            cJSON *COUNTflag = cJSON_GetObjectItem(spec_data,"COUNT");
            if(COUNTflag)Terminal.COUNTflag = COUNTflag->valueint;

            cJSON *PCRDflag = cJSON_GetObjectItem(spec_data,"PCRD");
            if(PCRDflag)Terminal.PCRDflag = PCRDflag->valueint;

            cJSON_Delete(spec_data);
        }
    }
    else perror("open /flag_info");
}

void WriteFlags(__u8 isPCRD,__u8 isCOUNT,__u8 flag)
{
    char *path = "/flag_info";
    char szBuf[256] = {0};
    int fd;
    if((fd = open(path,O_CREAT | O_RDWR,0600)) < 0)perror("open");
    while(flock(fd,LOCK_EX|LOCK_NB));
    lseek(fd,0,SEEK_SET);
    if(read(fd,szBuf,sizeof(szBuf)) == -1)perror("read");
    while(flock(fd,LOCK_UN));
    close(fd);

    cJSON *spec_data = cJSON_Parse(szBuf);
    if(!spec_data)spec_data = cJSON_CreateObject();

    if(isCOUNT)
    {
        cJSON *COUNTflag = cJSON_GetObjectItem(spec_data,"COUNT");
        if(COUNTflag)
            cJSON_ReplaceItemInObject(spec_data,"COUNT",cJSON_CreateNumber(flag));
        else
            cJSON_AddNumberToObject(spec_data, "COUNT", flag);
    }

    if(isPCRD)
    {
        cJSON *PCRDflag = cJSON_GetObjectItem(spec_data,"PCRD");
        if(PCRDflag)
            cJSON_ReplaceItemInObject(spec_data,"PCRD",cJSON_CreateNumber(flag));
        else
            cJSON_AddNumberToObject(spec_data, "PCRD", flag);
    }

    char *buf = cJSON_Print(spec_data);
    cJSON_Delete(spec_data);
    // ��д�����ļ�
    if((fd = open(path,O_CREAT | O_RDWR,0600)) < 0)perror("open");
    while(flock(fd,LOCK_EX|LOCK_NB));
    lseek(fd,0,SEEK_SET);
    if(write(fd,buf,strlen(buf)) == -1)perror("write");
    while(flock(fd,LOCK_UN));
    close(fd);
    free(buf);
}
