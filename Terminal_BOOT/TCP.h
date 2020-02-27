#ifndef _TCP_TERMINAL_H_
#define _TCP_TERMINAL_H_
#include "CJSON.h"

/******************************�����ն�״̬����*******************************/
#define NO_LOAD 			0x10    //δ��¼������
#define	LOAD_ON				0x11    //��¼�������ɹ�
/*****************************��̨ͨ�Ź����붨��*******************************/
#define SITE_LOAD               	0xc001  //վ���¼�ƶ�
#define SITE_LOAD_BACK          	0xc002  //վ���¼�ƶ˷���
/******************************�������������*********************************/
#define APP_ERROR_BACK                  0x10ff  //Ӧ�ó�����
#define GET_CONFIG                      0xf001  //��ȡ������
#define GET_CONFIG_BACK                 0xf002  //��ȡ�������
#define SET_CONFIG                      0xf101  //����������
#define SET_CONFIG_BACK                 0xf102  //�����������
#define EXIT_VIRTUAL_SERVER             0xff01  //�˳����������
/******************************BOOTLOAD����*******************************/
#define SCAN_VERSION                    0x1001  //�汾��ɨ��ָ��
#define SCAN_VERSION_BACK               0x1002  //�汾��ɨ��ָ���
#define ENTER_BOOTLOADER                0x1101  //������������ͽ���bootloadָ��
#define ENTER_BOOTLOADER_BACK           0x1102  //������������ͽ���bootloadָ���
#define UPDATE_DOWNLOAD                 0x1201  //��������
#define UPDATE_DOWNLOAD_BACK            0x1202  //�������ط���
#define UPDATE_WRITE                    0x1301  //����д��
#define UPDATE_WRITE_BACK               0x1302  //����д�뷵��
#define UPDATE_WRITE_BACK_FINISH        0x1303  //����д�����
/***************************վ��ͨ�Ź����붨��*******************************/
#define SI_LO               		0x01    //վ���¼�ƶ�
#define SI_LO_BACK          		0x02    //վ���¼�ƶ˷���
#define UNLOCK_KNEE        		0x11    //��׮ȡ��
#define UNLOCK_KNEE_BACK   		0x12    //��׮ȡ������
#define ACCOUNT_INFO        		0x13    //�˻���Ϣ
#define BI_IN              		0x21    //������׮
#define BI_IN_BACK          	  	0x22    //������׮����
#define CAN_BI_IN_BACK          	0x23    //CAN������׮����
#define HE_BEAT         	        0x31    //վ������
#define HE_BEAT_BACK                    0x32    //վ����������
#define CA_RENT             	  	0x41    //ˢ���⳵
#define CA_RENT_BACK        	  	0x42    //ˢ���⳵����
#define CA_RENT_AFFIRM      		0x43    //ˢ���⳵ȷ��
#define CA_RENT_AFFIRM_BACK      	0x44    //ˢ���⳵ȷ�Ϸ���
#define BI_AUTHORIZATION      		0x51    //������׮��Ȩ
#define BI_AUTHORIZATION_BACK 		0x52    //������׮��Ȩ����
#define CH_KNEE			   	0x61    //��ѯ׮��Ϣ
#define CH_KNEE_BACK 			0x62    //��ѯ׮��Ϣ����
#define BI_OUT			   	0x71    //ˢ����Ȩ��׮
#define BI_OUT_BACK 			0x72    //ˢ����Ȩ��׮����
#define BOOTLOAD_IN                     0x81    //������������ͽ���bootloadָ��
#define BOOTLOAD_OUT                    0x82    //��������������˳�bootloadָ��
#define ERROR_APP                       0x91    //Ӧ�ó������

#define PACKAGE_state                   0x00    //��̬

void TCP_KPD(int sockfd);
void TCP_SW(int sockfd);

#endif
