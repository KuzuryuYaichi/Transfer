#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
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
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/rtnetlink.h>
#include <linux/netlink.h>
#include <math.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <sys/msg.h>
#include "can.h"
#include "TCP.h"
#include "UDP.h"
#include "Charger_can.h"
#include "Terminal.h"
#include "CommonFunc.h"

//typedef enum CARD_TYPE {
//    USER_CARD = 0x01,
//    RESET_CARD,
//    NUM_CARD,
//    POWER_CARD,
//    MANAGE_CARD
//}CARD_TYPE;

typedef enum CARD_TYPE {
    USER_CARD = 0x00,
    POWER_CARD,
    RESET_CARD,
    MANAGE_CARD
}CARD_TYPE;

extern int CAN0_fd;
extern RUNNING_STATE isRunning;
extern struct Terminal_Info     Terminal;
extern struct Knee_struct       Knee_can[EQUIP_NUM_MAX+1];      //���ڼ�¼׮��CAN��������
extern struct Net_struct        Net_data[EQUIP_NUM_MAX+1];      //���ڼ�¼��������������
extern struct UdpScan_struct    UpScan_Scan[EQUIP_NUM_MAX + 1]; //���ڼ�¼׮��ɨ�践������
struct UpData_struct      UpData_data = {0};
/**************************************
�������ƣ�GetUpdateFile
�����������������ļ��ж�ȡ�ض�λ�õ�256���ַ�
���룺
�����
������Ϣ��
***************************************/
void GetUpdateFile(__u32 address)
{
    char szBuf[256] = {0};
    const char *src = "/update/program";
    UpData_data.program_addr = address;
    int fd;
    if((fd = open(src,O_CREAT | O_RDWR,0600)) > 0)
    {
        int filesize = lseek(fd,0,SEEK_END);
        printf("�ļ���filesize:%d �·���filesize:%d\r\n",filesize,UpData_data.program_size);
        lseek(fd,64 + UpData_data.program_addr,SEEK_SET);
        while(flock(fd,LOCK_EX|LOCK_NB));
        int readlen = read(fd,szBuf,sizeof(szBuf));
        while(flock(fd,LOCK_UN));
        printf("��ȡ��%d���ȵ��ļ�\r\n",readlen);
        memcpy(UpData_data.data,szBuf,256);
        close(fd);
    }
    else perror("open src");
}
__u8 errase_flag = 0,write_state = 0;
/**************************************
�������ƣ�
��������������CAN��������
���룺
�����
������Ϣ��
***************************************/
int RecvCanData(int sockfd)
{
    int queue_temp = -1;
    int temp = 0;

    struct timeval timeout={0,1000}; // ����1ms���յȴ���ʱ
    setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));

    struct can_frame CANRx = {0};
    struct can_frame *CANRx_p = &CANRx;

    struct Terminal_MsgSt CanRecv_queue = {0};
    CanRecv_queue.MsgType = IPC_NOWAIT;
    //��������
    if(recv(sockfd,CANRx_p,16,MSG_WAITALL) > 0)
    {
        __u32 function_code = (CANRx_p->can_id)&0xFC0;      //��׮ָ�����
        __u16 ID_code = (CANRx_p->can_id)&0x03F;            //׮λ��
        CanRecv_queue.MsgBuff[0] = ID_code;
        pthread_mutex_lock(&Knee_can[ID_code].mt);
        switch(function_code)
        {
            case CAN_MCU_VSON:    //���ذ汾��
            {
                printf("CAN ���ذ汾�ŷ���\r\n");
                UpScan_Scan[ID_code].MS = (CANRx_p->data[0] << 24) + (CANRx_p->data[1] << 16) + (CANRx_p->data[2] << 8) + CANRx_p->data[3];
                UpScan_Scan[ID_code].MH = (CANRx_p->data[4] << 24) + (CANRx_p->data[5] << 16) + (CANRx_p->data[6] << 8) + CANRx_p->data[7];
                CanRecv_queue.MsgBuff[1] = EQUIP_MCU_BACK;
                queue_temp = msgsnd(CanRecv_qid, &CanRecv_queue, 2, IPC_NOWAIT);
                break;
            }
            case CAN_DISP_VSON:   //�Կذ汾��
            {
                printf("CAN �Կذ汾�ŷ���\r\n");
                UpScan_Scan[ID_code].DS = (CANRx_p->data[0] << 24) + (CANRx_p->data[1] << 16) + (CANRx_p->data[2] << 8) + CANRx_p->data[3];
                UpScan_Scan[ID_code].DH = (CANRx_p->data[4] << 24) + (CANRx_p->data[5] << 16) + (CANRx_p->data[6] << 8) + CANRx_p->data[7];
                CanRecv_queue.MsgBuff[1] = EQUIP_DIS_BACK;
                queue_temp = msgsnd(CanRecv_qid, &CanRecv_queue, 2, IPC_NOWAIT);
                break;
            }
            case CAN_CARRIER_VSON: //�ز��汾��
            {
                printf("CAN ɨ���ز��汾�ŷ���\r\n");
                UpScan_Scan[ID_code].CS = (CANRx_p->data[0] << 24) + (CANRx_p->data[1] << 16) + (CANRx_p->data[2] << 8) + CANRx_p->data[3];
                UpScan_Scan[ID_code].CH = (CANRx_p->data[4] << 24) + (CANRx_p->data[5] << 16) + (CANRx_p->data[6] << 8) + CANRx_p->data[7];
                CanRecv_queue.MsgBuff[1] = EQUIP_CARRIER_BACK;
                queue_temp = msgsnd(CanRecv_qid, &CanRecv_queue, 2, IPC_NOWAIT);
                break;
            }
            case CAN_LOGO_VSON: //LOGO�汾��
            {
                printf("CAN ɨ��LOGO�汾�ŷ���\r\n");
                UpScan_Scan[ID_code].LS = (CANRx_p->data[0] << 24) + (CANRx_p->data[1] << 16) + (CANRx_p->data[2] << 8) + CANRx_p->data[3];
                UpScan_Scan[ID_code].LH = (CANRx_p->data[4] << 24) + (CANRx_p->data[5] << 16) + (CANRx_p->data[6] << 8) + CANRx_p->data[7];
                CanRecv_queue.MsgBuff[1] = EQUIP_LOGO_BACK;
                queue_temp = msgsnd(CanRecv_qid, &CanRecv_queue, 2, IPC_NOWAIT);
                break;
            }
            case CAN_PCRD_VSON: //�ϵ縴λ���汾��
            {
                printf("CAN ɨ��ϵ縴λ���汾�ŷ���\r\n");
                UpScan_Scan[ID_code].PS = (CANRx_p->data[0] << 24) + (CANRx_p->data[1] << 16) + (CANRx_p->data[2] << 8) + CANRx_p->data[3];
                UpScan_Scan[ID_code].PH = (CANRx_p->data[4] << 24) + (CANRx_p->data[5] << 16) + (CANRx_p->data[6] << 8) + CANRx_p->data[7];
                CanRecv_queue.MsgBuff[1] = EQUIP_PCRD_BACK;
                queue_temp = msgsnd(CanRecv_qid, &CanRecv_queue, 2, IPC_NOWAIT);
                break;
            }
            case CAN_CARD:        //0x300
            {
                for(temp=0;temp<4;temp++)
                {
                    Knee_can[ID_code].card_ID[temp] = CANRx_p->data[temp];
                }
                printf("����\r\n");
                Knee_can[ID_code].function = CANRx_p->data[4];
                Knee_can[ID_code].recv_count |= 0x01;
                Knee_can[ID_code].card_type = CANRx_p->data[5];
                if(Knee_can[ID_code].card_type == POWER_CARD)//�ϵ翨
                {
                    printf("�ϵ翨\r\n");
                    struct can_frame CANTx = {0};
                    struct can_frame *CANTx_p = &CANTx;
                    memset(CANTx_p,0,16);
                    CANTx_p->data[0] = 0x08;
                    CANTx_p->data[1] = 0x01;//��־λ��λ ǿ�ƶϵ�
                    CAN_send(CANTx_p,sockfd,CAN_BOARD);
                    usleep(10000);
                    CAN_send(CANTx_p,sockfd,CAN_BOARD);
                    usleep(10000);
                    CAN_send(CANTx_p,sockfd,CAN_BOARD);
                }
                else if(Knee_can[ID_code].card_type == RESET_CARD)//��λ��
                {
                    printf("��λ��\r\n");
                    system("reboot");
                }
                break;
            }
            case CAN_SITE_ID:   //0x4C0
            {
                CanRecv_queue.MsgBuff[1] = SITE_ID_CHECK;
                queue_temp = msgsnd(UpdataRecv_qid, &CanRecv_queue, 2, IPC_NOWAIT);
                printf("վ��ID��ѯ ׮��:%d\r\n",ID_code);
                break;
            }
            case CAN_HEART:     //0x440
            {
                Knee_can[ID_code].knee_ID = Hex_Dec(CANRx_p->data,7);  //�ṹ���б���׮��SN��
                printf("%d��׮������ %lld\r\n",ID_code,Knee_can[ID_code].knee_ID);
                break;
            }
            case CAN_BOOTLOAD:  //0x040
            {
                if(CANRx_p->data[0] == 0x00)
                {
                    struct Terminal_MsgSt CANUpdataRecv_queue = {0};
                    CANUpdataRecv_queue.MsgType = IPC_NOWAIT;
                    if(write_state == 0)
                    {
                        printf("����Bootloader\r\n");         //����Bootloader������Ͳ���ָ��
                        CANUpdataRecv_queue.MsgBuff[0] = ID_code;
                        CANUpdataRecv_queue.MsgBuff[1] = ERRASE;
                        msgsnd(UpdataRecv_qid, &CANUpdataRecv_queue, 2, IPC_NOWAIT);

                        CANUpdataRecv_queue.MsgBuff[1] = UPDATE_WRITE_CAN_BACK;
                        CANUpdataRecv_queue.MsgBuff[0] = 1;
                        msgsnd(CanRecv_qid, &CANUpdataRecv_queue, 2, IPC_NOWAIT);
                    }
                    else if(write_state == 1)
                    {
                        printf("�˳�Bootloader\r\n");
                        CANUpdataRecv_queue.MsgBuff[0] = ID_code;
                        CANUpdataRecv_queue.MsgBuff[1] = UPDATE_WRITE_CAN_FINISH;
                        queue_temp = msgsnd(CanRecv_qid, &CANUpdataRecv_queue, 2, IPC_NOWAIT);
                    }
                }
                else
                {
                    printf("δ�ܽ���Bootloader\r\n");
                }
                break;
            }
            case CAN_ERRASE:      //0x080
            {
                if(CANRx_p->data[0] == 0x00)
                {
                    printf("�������\r\n");
                }
                else
                {
                    printf("����ʧ��\r\n");
                }
                errase_flag = 1;
                GetUpdateFile(0);
                CanRecv_queue.MsgBuff[0] = ID_code;
                CanRecv_queue.MsgBuff[1] = PROGRAM_WRITE;
                msgsnd(UpdataRecv_qid, &CanRecv_queue, 2, IPC_NOWAIT);

                CanRecv_queue.MsgBuff[1] = UPDATE_WRITE_CAN_BACK;
                CanRecv_queue.MsgBuff[0] = 1;
                msgsnd(CanRecv_qid, &CanRecv_queue, 2, IPC_NOWAIT);
                break;
            }
            case CAN_WRITE:       //0x0C0
            {
                __u32 address = (CANRx_p->data[0]<<24)+(CANRx_p->data[1]<<16)+(CANRx_p->data[2]<<8)+CANRx_p->data[3];
                if(CANRx_p->data[4] == 0x25)
                {
                    if(address/256 >= (UpData_data.program_size - 64)/256)      //������д���
                    {
                        struct Terminal_MsgSt UpdataRecv_queue = {0};
                        UpdataRecv_queue.MsgType = IPC_NOWAIT;
                        UpdataRecv_queue.MsgBuff[0] = ID_code;
                        UpdataRecv_queue.MsgBuff[1] = BOOT_OUT;
                        msgsnd(UpdataRecv_qid, &UpdataRecv_queue, 2, IPC_NOWAIT);
                    }
                    else
                    {
                        GetUpdateFile(address + 256);
                        CanRecv_queue.MsgBuff[0] = ID_code;
                        CanRecv_queue.MsgBuff[1] = PROGRAM_WRITE;
                        queue_temp = msgsnd(UpdataRecv_qid, &CanRecv_queue, 2, IPC_NOWAIT);

                        static __u8 progress_last = 0;
                        __u8 progress_now = 100*address/UpData_data.program_size;
                        printf("CAN_WRITE���ؽ���:%d",progress_now);
                        if(progress_last != progress_now)
                        {
                            CanRecv_queue.MsgBuff[1] = UPDATE_WRITE_CAN_BACK;
                            CanRecv_queue.MsgBuff[0] = progress_last = progress_now;
                            queue_temp = msgsnd(CanRecv_qid, &CanRecv_queue, 2, IPC_NOWAIT);
                        }
                    }
                }
                else if(CANRx_p->data[4] == 0x26)
                {
                    struct timeval timeout={0,200000};  //��ʱ200ms
                    select(0,NULL,NULL,NULL,&timeout);

                    if(address/256 > (UpData_data.program_size - 64)/256)      //�����ַԽ�����ͷ��ʼ
                    {
                        printf("��ַԽ�����·���ȫ������\r\n");
                        CanRecv_queue.MsgBuff[0] = ID_code;
                        CanRecv_queue.MsgBuff[1] = ERRASE;
                        queue_temp = msgsnd(UpdataRecv_qid, &CanRecv_queue, 2, IPC_NOWAIT);
                    }
                    else
                    {
                        printf("���·��͵�ǰ���ݰ�\r\n");
                        GetUpdateFile(address);
                        CanRecv_queue.MsgBuff[0] = ID_code;
                        CanRecv_queue.MsgBuff[1] = PROGRAM_WRITE;
                        queue_temp = msgsnd(UpdataRecv_qid, &CanRecv_queue, 2, IPC_NOWAIT);

                        CanRecv_queue.MsgBuff[1] = UPDATE_WRITE_CAN_BACK;
                        CanRecv_queue.MsgBuff[0] = 100*address/UpData_data.program_size;
                        queue_temp = msgsnd(CanRecv_qid, &CanRecv_queue, 2, IPC_NOWAIT);
                    }
                }
                break;
            }
            default:
            {
                break;
            }
        }
        pthread_mutex_unlock(&Knee_can[ID_code].mt);
    }
    return 0;
}

pthread_t timeout_tid;
void* timeout_thread(void* arg)
{
    pthread_detach(pthread_self());
    __u8 equip_num = *(__u8*)arg;
    free(arg);
    printf("��ʱ�߳� equip_num:%d\r\n",equip_num);
    errase_flag = 0;
    int i;
    for(i=0;i<2;i++)
    {
        struct timeval tm = {3,500000};
        select(0,NULL,NULL,NULL,&tm);
        printf("errase_flag is %d\r\n",errase_flag);
        if(!errase_flag)
        {
            struct Terminal_MsgSt ter = {0};
            ter.MsgType = IPC_NOWAIT;
            ter.MsgBuff[0] = equip_num;
            ter.MsgBuff[1] = ERRASE;
            msgsnd(UpdataRecv_qid, &ter, 2, IPC_NOWAIT);
            printf("��ʱ���� CAN�߳�\r\n");
        }
    }
}
/**************************************
�������ƣ�
����������Ӧ�ó���CAN���߷�������
���룺
�����
������Ϣ��
***************************************/
int CanSendData(int sockfd)
{
    __u32 t;
    __u32 cansend_ID;
    struct can_frame CANTx = {0};
    struct can_frame *CANTx_p = &CANTx;
    /*************��������************/
    struct Terminal_MsgSt CANUpdataSend_queue = {0};
    CANUpdataSend_queue.MsgType = IPC_NOWAIT;
    int temp = msgrcv(UpdataRecv_qid, &CANUpdataSend_queue, 2, 0, IPC_NOWAIT);
    if(temp >= 0)
    {
        __u8 equip_num = CANUpdataSend_queue.MsgBuff[0];
        __u8 order_num = CANUpdataSend_queue.MsgBuff[1];
        switch(order_num)
        {
            case EQUIP_ACCEPT:
            {
                printf("EQUIP_ACCEPT\r\n");
                cansend_ID = CAN_ORDER + 0;   //�㲥
                CANTx_p->data[0] = 0x03;      //׮������״̬��ѯ
                CANTx_p->data[1] = 0x00;
                CANTx_p->data[2] = 0x00;
                CAN_send(CANTx_p,sockfd,cansend_ID);
                break;
            }
            case EQUIP_MCU:
            {
                printf("ɨ�����ذ汾��\r\n");
                cansend_ID = CAN_MCU_VSON + 0;
                CAN_send(CANTx_p,sockfd,cansend_ID);
                break;
            }
            case EQUIP_DIS:
            {
                printf("ɨ���Կذ汾��\r\n");
                cansend_ID = CAN_DISP_VSON + 0;
                CAN_send(CANTx_p,sockfd,cansend_ID);
                break;
            }
            case EQUIP_CARRIER:
            {
                printf("ɨ���ز��汾��\r\n");
                cansend_ID = CAN_CARRIER_VSON + 0;
                CAN_send(CANTx_p,sockfd,cansend_ID);
                break;
            }
            case EQUIP_LOGO:
            {
                printf("ɨ��LOGO�汾��\r\n");
                cansend_ID = CAN_LOGO_VSON + 0;
                CAN_send(CANTx_p,sockfd,cansend_ID);
                break;
            }
            case EQUIP_PCRD:
            {
                printf("ɨ��ϵ縴λ���汾��\r\n");
                cansend_ID = CAN_PCRD_VSON + 63;
                CAN_send(CANTx_p,sockfd,cansend_ID);
                break;
            }
            case SITE_ID_CHECK:
            {
                printf("վ��ID��ѯ���� ׮��:%d\r\n",equip_num);
                cansend_ID = CAN_SITE_ID + equip_num;
                Dec2Hex(Terminal.SITE_ID * 10,CANTx_p->data);
                CAN_send(CANTx_p,sockfd,cansend_ID);
                break;
            }
            case BOOT_IN:
            {
                write_state = 0;
                printf("���ͽ���Bootloader����\r\n");
                cansend_ID = CAN_BOOTLOAD + equip_num;
                CANTx_p->data[0] = UpData_data.equip_name;  //��������
                CANTx_p->data[1] = equip_num;               //׮λ��
                CANTx_p->data[2] = 0x01;                    //����Bootloader
                CAN_send(CANTx_p,sockfd,cansend_ID);

                int j;
                for(j=0;j<2;j++)
                {
                    struct timeval timeout={0,100000};
                    select(0,NULL,NULL,NULL,&timeout);
                    CAN_send(CANTx_p , sockfd , cansend_ID);
                }

                break;
            }
            case ERRASE:
            {
                printf("CAN ERRASE\r\n");

                printf("���Ͳ�������\r\n");
                cansend_ID = CAN_ERRASE + equip_num;
                CANTx_p->data[0] = 0;
                CANTx_p->data[1] = 0;
                CANTx_p->data[2] = 0;
                CANTx_p->data[3] = 0;
                CANTx_p->data[4] = 0xFF;
                CANTx_p->data[5] = 0xFF;
                CANTx_p->data[6] = 0xEB;
                CANTx_p->data[7] = 0x90;
                CAN_send(CANTx_p,sockfd,cansend_ID);

                __u8 *arg = (__u8*)malloc(sizeof(__u8));
                if(arg)
                {
                    *arg = equip_num;
                    if(pthread_create(&timeout_tid,NULL,timeout_thread,(void*)arg))
                    {
                        free(arg);
                        perror("Create Timeout thread error!");
                    }
                }
                break;
            }
            case PROGRAM_WRITE:
            {
//              printf("PROGRAM_WRITE\r\n");
                int page_num = 1;
                __u16 sum = 0;
                cansend_ID = CAN_WRITE + equip_num;
                CANTx_p->data[0] = (UpData_data.program_addr >> 24) & 0xFF; //������д��ʼ��ַ
                CANTx_p->data[1] = (UpData_data.program_addr >> 16) & 0xFF;
                CANTx_p->data[2] = (UpData_data.program_addr >> 8) & 0xFF;
                CANTx_p->data[3] = (UpData_data.program_addr) & 0xFF;
                CANTx_p->data[4] = 0x25;
                CANTx_p->data[5] = 0x16;
                CANTx_p->data[6] = 0x59;
                CANTx_p->data[7] = 0x88;
                CAN_send(CANTx_p,sockfd,cansend_ID);    //����д��ָ��
                //�������ݰ���ҳд��
                memset(CANTx_p,0,16);
                for(page_num = 1;page_num < 37;page_num++)  //д�����1~36ҳ
                {
                    CANTx_p->data[0] = page_num;
                    for(t=1;t<=7;t++)
                    {
                        CANTx_p->data[t] = UpData_data.data[(page_num-1)*7+t-1];
                        sum += CANTx_p->data[t];
                    }
                    if(CAN_send(CANTx_p, sockfd, cansend_ID))
                    {
                        printf("��%dҳд��ʧ��\r\n",page_num);
                    }
                }
                if(page_num == 37)                          //д�����37ҳ
                {
                    memset(CANTx_p,0,16);
                    CANTx_p->data[0] = page_num;
                    for(t=1;t<=4;t++)
                    {
                        CANTx_p->data[t] = UpData_data.data[(page_num-1)*7+t-1];
                        sum += CANTx_p->data[t];
                    }
                    CANTx_p->data[5] = (sum>>8)&0xFF;
                    CANTx_p->data[6] = sum&0xFF;
                    CANTx_p->data[7] = 0x6F;
                    if(!CAN_send(CANTx_p,sockfd,cansend_ID))
                    {
//                      printf("��37ҳ����\r\n");
                    }
                }
                break;
            }
            case BOOT_OUT:
            {
                write_state = 1;
                cansend_ID = CAN_BOOTLOAD + equip_num;
                CANTx_p->data[0] = UpData_data.equip_name;
                CANTx_p->data[1] = equip_num;
                CANTx_p->data[2] = 0x02;
                CAN_send(CANTx_p,sockfd,cansend_ID);
                break;
            }
            default:
            {
                break;
            }
        }
    }
}
