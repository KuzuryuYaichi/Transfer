#ifndef _UDP_TERMINAL_H_
#define _UDP_TERMINAL_H_

//�������Ͷ���
#define TERMINAL			0x01                    //�����ն�
#define ALUM_MCU                        0x02                    //˫��׮����
#define ALUM_DISPLAY			0x03                    //˫��׮�Կ�
#define CARRIER_MCU			0x04                    //�ز�ͨ������
#define ASCII_LOGO                      0x05                    //�ֿ�
/************************************/
//�����붨��
#define	EQUIP_ACCEPT			0xA1                    //��ȡ�����豸
#define	EQUIP_ACCEPT_BACK		0xB1                    //��ȡ����Ӧ��
#define	EQUIP_MCU			0xE1                    //��ȡ�����豸
#define	EQUIP_MCU_BACK                  0xE2                    //��ȡ����Ӧ��
#define	EQUIP_DIS			0xE3                    //��ȡ�Կ��豸
#define	EQUIP_DIS_BACK                  0xE4                    //��ȡ�Կ�Ӧ��
#define	EQUIP_CARRIER			0xE5                    //��ȡ�ز��豸
#define	EQUIP_CARRIER_BACK		0xE6                    //��ȡ�ز�Ӧ��
#define	EQUIP_LOGO			0xE7                    //��ȡ�ֿ�
#define	EQUIP_LOGO_BACK                 0xE8                    //��ȡ�ֿ�Ӧ��
#define	EQUIP_PCRD			0xE9                    //��ȡ�ϵ縴λ��
#define	EQUIP_PCRD_BACK                 0xEA                    //��ȡ�ϵ縴λ��Ӧ��
#define	ERRASE				0xA2			//����
#define	ERRASE_BACK			0xB2			//����Ӧ��
#define	PROGRAM_WRITE			0xA3			//�����д��
#define	PROGRAM_WRITE_BACK		0xB3			//�����д��Ӧ��
#define	UPDATE_WRITE_CAN                0xA7			//�����д��
#define	UPDATE_WRITE_CAN_BACK		0xB7			//�����д��Ӧ��
#define UPDATE_WRITE_CAN_FINISH         0xB8                    //�����д�����
#define UPDATE_WRITE_CAN_FAIL           0xBC                    //���������ʧ��
#define BOOT_IN				0xA4			//����Bootloader
#define	BOOT_OUT			0xA5			//�˳�Bootloader
#define	PROGRAM_WRITE_FINISH            0xA6			//����д�����
#define	SITE_ID_WRITE                   0xA8			//д��վ���
#define	EQUIP_MAINTAIN_ENTER		0xA9			//վ�����ά��״̬
#define	EQUIP_MAINTAIN_EXIT		0xAA			//վ���˳�ά��״̬
#define	NET_DISCONNECT_ENTER		0xB9			//վ�����������ж�
#define	NET_DISCONNECT_EXIT		0xBA			//վ���������ӻָ�
#define IMMEDIATE_UNLOCK                0xAB                    //Ӧ������

#define	SITE_DNS_WRITE                  0xAD			//д������
#define	SITE_PASSWORD_WRITE             0xAE			//д��վ������
#define	WIFI_NAME_WRITE                 0xBD			//д��Wifi��
#define	WIFI_PASSWORD_WRITE             0xBE			//д��Wifi����
#define	SERVER_PORT_WRITE               0xD1			//д��������˿�
#define	SERVER_IP_WRITE                 0xD2			//д�������IP
#define	RESET_COUNT_WRITE               0xD3                    //������������
#define	SUCC_COUNT_WRITE                0xD4                    //��½�ɹ���������
#define	FAIL_COUNT_WRITE                0xD5                    //��½ʧ�ܴ�������
#define	BACKSTAGE_SELECT                0xD6                    //��½��̨ѡ��
#define	CUSTOMER_ID_WRITE               0xD7			//д��ͻ����

#define	SINGLE_NET                      0xAF			//������������л�
#define	SINGLE                          0xBF			//������
#define	NET                             0xCF			//�����
/************************************/

#endif
