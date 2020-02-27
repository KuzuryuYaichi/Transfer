#ifndef _TERMINAL_TERMINAL_H_
#define _TERMINAL_TERMINAL_H_

#define EQUIP_NUM_MAX 64            //�����׮��

#define LIGHT_OFF 0
#define LIGHT_ON 1
#define LIGHT_BLINK 2

extern int CanRecv_qid;
extern int UpdataRecv_qid;

/***********************************************************/
//׮����Ϣ
struct Knee_struct {                //׮�����ݽṹ��
    __u64 knee_ID;                  //���׮ID��(10����)
    __u8 card_type;                 //��������  0x00 �û�����0x03 ����Ա��
    __u8 card_ID[4];
    __u8 bike_ID[7];
    __u8 mal_infor[5];              //������Ϣ
    __u8 charge_infor[5];           //�����Ϣ
    int heart_count;                //��������ͳ��
    __u8 recv_count;                //�����жϽ賵/������bit0:���յ�0x300ָ�bit1:���յ�0x340ָ�bit2:���յ�0x380ָ��
    __u8 function;                  //����λ��0x8��ѯ��0x4�賵
    __u64 additional;               //����ֵ
    __u64 open_type;                //������ʽ
    __u64 additional_value;         //���������ֵ
    pthread_mutex_t mt;             //�߳�ͬ���������
};
/***********************************************************/
struct UpData_struct {              //��������UDP���ݽṹ��
    __u8 equip_name;                //��������
    char data[256];                 //��������(286�ֽ�)
    __u32 program_addr;             //����д����ʼ��ַ
    __u32 program_size;             //��������
};
/***********************************************************/
struct UdpScan_struct {             //ɨ�践������
   __u32 MH;                        //����Ӳ���汾��
   __u32 MS;                        //��������汾��
   __u32 DH;                        //�Կ�Ӳ���汾��
   __u32 DS;                        //�Կ�����汾��
   __u32 CH;                        //�ز�Ӳ���汾��
   __u32 CS;                        //�ز�����汾��
   __u32 LH;                        //�ֿ�Ӳ���汾��
   __u32 LS;                        //�ֿ�����汾��
   __u32 PH;                        //�ϵ縴λ��Ӳ���汾��
   __u32 PS;                        //�ϵ縴λ������汾��
   pthread_mutex_t mt;              //�߳�ͬ���������
};
/********************��������������************************/
struct Net_struct {
    __u16 function_type;            //ָ�����ͣ�ˢ���賵��������ɨ��賵
    __u64 site_ID;                  //վ��ID
    __u64 knee_ID;                  //��׮ID
    int result;                     //ִ�н��
    int account;                    //�˻����
    char *account_str;              //�ַ��������˻����
    int consumption;                //���ѽ��
    char *rent_begin;               //��ʼ�⳵ʱ��
    int time_length;                //�⳵ʱ��
    pthread_mutex_t mt;             //�߳�ͬ���������
};
/*********************��Ϣ�ṹ��********************/
struct Terminal_MsgSt {
    long int MsgType;               //����Ϣ���������߿�ʱ =0,һֱ�ȴ�; IPC_NOWAIT,��������-1
    char MsgBuff[2];                //ǰ���ֽ����ڴ�Ź����룬�����ֽڴ��׮λ��
};
/*******************��ѯ׮��Ϣ�ṹ��*****************/
struct CheckKnee_Struct {
    __u8 count;                     //��ѯ����
    __u8 check_flag;                //��ѯ������Ϣ��־
    __u8 isUser;                    //�Ƿ�Ϊ�û�
    __u64 bike_id;                  //��ǰ����
};
/******************�����ն�������Ϣ��Ϣ�ṹ��******************/
struct Terminal_Info {
    int Customer_ID;                //�ͻ����
    char Hardware_version[10];      //Ӳ���汾
    char Software_version[10];      //����汾
    __u64 SITE_ID;                  //վ��ID
    char SALT_site[10];             //վ����ֵ
    char SALT_password[32];         //վ������
    char Server[32];                //վ������
    char Server_IP[32];             //������IP
    int Server_Port;                //������PORT
    char Wifi_password[32];         //Wifi����
    char Wifi_name[32];             //Wifi��
    char TOKEN_site[128];           //վ��ͨѶtoken
    __u64 ConnectCount;             //��������
    __u64 DisConnectCount;          //���ߴ���
    __u64 ResetCount;               //��������
    int BackStage_Sel;              //���Ӻ�̨
};
/******************������ת״̬********************
�ڽ���Bootloaderʱ��Ҫ�ж��Ƿ��ȡAPPFLAG Ҫ��Ҫ��ʼ��ATģ��
�ڽ������������ʱ��Ҫ�ж�Ҫ��Ҫ��ʼ��ATģ��
�ڽ�����ʵ������ʱ����Ҫ�ж�
�ϵ�ֱ�ӽ���Bootloader ��ʱ��Ҫ�ж�APPFLAG ���������������Ҫ��ʼ��ATģ��
���������ָ�����Bootloader ��ʱ����Ҫ�ж�APPFLAG �����������������Ҫ��ʼ��ATģ��
�ϵ縴λ����Bootloader�������������� ��ʱ��Ҫ�ж��Ƿ����ӹ���������� ��Ҫ��ʼ��ATģ��
���������ָ�����Bootloader�������������� ��ʱ��Ҫ�ж�APPFLAG ���������������Ҫ��ʼ��ATģ��
���������������ʵ������ ��Ҫ��ʼ��ATģ��
************************************************/
typedef enum RUNNING_STATE{
    POWER_OFF = '0',
    PROGRAM_BOOTLOADER,
    PROGRAM_VIRTUAL_SERVER,
    PROGRAM_ACTUAL_SERVER
}RUNNING_STATE;
/***********************����������ת�Ĵ��θ�ʽ****************************
��һ������ ��������ת(�� BootloaderĬ���ϵ� VirutalServerĬ��Bootloader RealServerĬ��VirtualServer)
�ڶ������� ��������ת(�� ��������)
���������� �Ƿ���Ҫ��ʼ��ATģ��(�� Bootloader Ĭ����Ҫ)
���ĸ����� ����Ѿ���ʼ����ATҪ����SITE_COMMON_TOKEN
**********************************************************************/
typedef enum INIT_AT{
    NOT_TO_INIT = '0',
    NEED_TO_INIT
}INIT_AT;

#endif
