#ifndef _UDP_TERMINAL_H_
#define _UDP_TERMINAL_H_

//程序类型定义
#define TERMINAL			0x01                    //数据终端
#define ALUM_MCU                        0x02                    //双铝桩主控
#define ALUM_DISPLAY			0x03                    //双铝桩显控
#define CARRIER_MCU			0x04                    //载波通信主控
#define ASCII_LOGO                      0x05                    //字库
/************************************/
//功能码定义
#define	EQUIP_ACCEPT			0xA1                    //获取在线设备
#define	EQUIP_ACCEPT_BACK		0xB1                    //获取在线应答
#define	EQUIP_MCU			0xE1                    //获取主控设备
#define	EQUIP_MCU_BACK                  0xE2                    //获取主控应答
#define	EQUIP_DIS			0xE3                    //获取显控设备
#define	EQUIP_DIS_BACK                  0xE4                    //获取显控应答
#define	EQUIP_CARRIER			0xE5                    //获取载波设备
#define	EQUIP_CARRIER_BACK		0xE6                    //获取载波应答
#define	EQUIP_LOGO			0xE7                    //获取字库
#define	EQUIP_LOGO_BACK                 0xE8                    //获取字库应答
#define	EQUIP_PCRD			0xE9                    //获取断电复位器
#define	EQUIP_PCRD_BACK                 0xEA                    //获取断电复位器应答
#define	ERRASE				0xA2			//擦除
#define	ERRASE_BACK			0xB2			//擦除应答
#define	PROGRAM_WRITE			0xA3			//程序包写入
#define	PROGRAM_WRITE_BACK		0xB3			//程序包写入应答
#define	UPDATE_WRITE_CAN                0xA7			//程序包写入
#define	UPDATE_WRITE_CAN_BACK		0xB7			//程序包写入应答
#define UPDATE_WRITE_CAN_FINISH         0xB8                    //程序包写入完成
#define UPDATE_WRITE_CAN_FAIL           0xBC                    //程序包升级失败
#define BOOT_IN				0xA4			//进入Bootloader
#define	BOOT_OUT			0xA5			//退出Bootloader
#define	PROGRAM_WRITE_FINISH            0xA6			//程序写入完成
#define	SITE_ID_WRITE                   0xA8			//写入站点号
#define	EQUIP_MAINTAIN_ENTER		0xA9			//站点进入维护状态
#define	EQUIP_MAINTAIN_EXIT		0xAA			//站点退出维护状态
#define	NET_DISCONNECT_ENTER		0xB9			//站点网络连接中断
#define	NET_DISCONNECT_EXIT		0xBA			//站点网络连接恢复
#define IMMEDIATE_UNLOCK                0xAB                    //应急开锁

#define	SITE_DNS_WRITE                  0xAD			//写入域名
#define	SITE_PASSWORD_WRITE             0xAE			//写入站点密码
#define	WIFI_NAME_WRITE                 0xBD			//写入Wifi名
#define	WIFI_PASSWORD_WRITE             0xBE			//写入Wifi密码
#define	SERVER_PORT_WRITE               0xD1			//写入服务器端口
#define	SERVER_IP_WRITE                 0xD2			//写入服务器IP
#define	RESET_COUNT_WRITE               0xD3                    //重启次数重置
#define	SUCC_COUNT_WRITE                0xD4                    //登陆成功次数重置
#define	FAIL_COUNT_WRITE                0xD5                    //登陆失败次数重置
#define	BACKSTAGE_SELECT                0xD6                    //登陆后台选择
#define	CUSTOMER_ID_WRITE               0xD7			//写入客户编号

#define	SINGLE_NET                      0xAF			//单机版网络版切换
#define	SINGLE                          0xBF			//单机版
#define	NET                             0xCF			//网络版
/************************************/

#endif
