#ifndef _CHARGER_CAN_TERMINAL_H_
#define _CHARGER_CAN_TERMINAL_H_

//��׮����״̬
#define    RENT     		0x91      //�賵
#define    REPAY    		0x92      //����
#define    INQUIRE  		0x93      //��ѯ
#define    HEART    		0x94      //����
#define    AFFIRM    		0x95      //�賵ȷ��
#define    CLOUD_AFFIRM         0x96      //ɨ��賵ȷ��
#define    CHECK     		0x97      //��̨������ѯ
#define    OUT     		0x98      //ˢ����Ȩ��׮
#define    OUT_AFFIRM     	0x99      //ˢ����Ȩ��׮ȷ��
#define    KNEE_ERROR     	0x9A      //����
#define    OUT_TIMEOUT          0x9B      //ȡ����ʱ
#define    LOW_BATTERY          0x9C      //��������
#define    GET_CONFIG_RESULT    0x9D      //��ȡ����
#define    SET_CONFIG_RESULT    0x9E      //д������
#define    SITE_ID_CHECK        0x9F      //վ��ID��ѯ
/***************************************/
//��׮ָ�����
#define    CAN_BOOTLOAD 	0x040     //Bootload����
#define    CAN_ERRASE   	0x080     //����
#define    CAN_WRITE    	0x0C0     //д��
#define    CAN_MCU_VSON         0x140     //���ذ汾��
#define    CAN_DISP_VSON        0x180     //�Կذ汾��
#define    CAN_PRODUCT_TIME	0x1C0     //��������
#define    CAN_CARRIER_VSON     0x200     //�ز��汾��
#define    CAN_LOGO_VSON        0x240     //LOGO�汾��
#define    CAN_PCRD_VSON        0x280     //�ϵ縴λ���汾��
#define    CAN_CARD     	0x300     //����
#define    CAN_KNEE     	0x340     //׮��
#define    CAN_BIKE     	0x380     //����
#define    CAN_CHARGE   	0x3C0     //���
#define    CAN_MAL      	0x400     //����
#define    CAN_HEART    	0x440     //����
#define    CAN_CHECK    	0x480     //��ѯ
#define    CAN_SITE_ID    	0x4C0     //��ȡվ���
#define    CAN_BOARD    	0x600     //�㲥(��ʱ�������ȵ���)
#define    CAN_ORDER    	0x640     //ָ��
#define    CAN_INFORM    	0x680     //�˻���Ϣ
#define    CAN_EXPENSE    	0x6C0     //������Ϣ
/***************************************/

int CanSendData(int sockfd);
int RecvCanData(int sockfd);

#endif
