#ifndef _TCP_TERMINAL_H_
#define _TCP_TERMINAL_H_
#include "CJSON.h"

/******************************数据终端状态定义*******************************/
#define NO_LOAD 			0x10    //未登录服务器
#define	LOAD_ON				0x11    //登录服务器成功
/*****************************后台通信功能码定义*******************************/
#define SITE_LOAD               	0xc001  //站点登录云端
#define SITE_LOAD_BACK          	0xc002  //站点登录云端返回
/******************************虚拟服务器配置*********************************/
#define APP_ERROR_BACK                  0x10ff  //应用程序损坏
#define GET_CONFIG                      0xf001  //读取配置项
#define GET_CONFIG_BACK                 0xf002  //读取配置项返回
#define SET_CONFIG                      0xf101  //设置配置项
#define SET_CONFIG_BACK                 0xf102  //设置配置项返回
#define EXIT_VIRTUAL_SERVER             0xff01  //退出虚拟服务器
/******************************BOOTLOAD流程*******************************/
#define SCAN_VERSION                    0x1001  //版本号扫描指令
#define SCAN_VERSION_BACK               0x1002  //版本号扫描指令返回
#define ENTER_BOOTLOADER                0x1101  //虚拟服务器发送进入bootload指令
#define ENTER_BOOTLOADER_BACK           0x1102  //虚拟服务器发送进入bootload指令返回
#define UPDATE_DOWNLOAD                 0x1201  //程序下载
#define UPDATE_DOWNLOAD_BACK            0x1202  //程序下载返回
#define UPDATE_WRITE                    0x1301  //程序写入
#define UPDATE_WRITE_BACK               0x1302  //程序写入返回
#define UPDATE_WRITE_BACK_FINISH        0x1303  //程序写入完成
/***************************站点通信功能码定义*******************************/
#define SI_LO               		0x01    //站点登录云端
#define SI_LO_BACK          		0x02    //站点登录云端返回
#define UNLOCK_KNEE        		0x11    //开桩取车
#define UNLOCK_KNEE_BACK   		0x12    //开桩取车返回
#define ACCOUNT_INFO        		0x13    //账户信息
#define BI_IN              		0x21    //车辆入桩
#define BI_IN_BACK          	  	0x22    //车辆入桩返回
#define CAN_BI_IN_BACK          	0x23    //CAN车辆入桩返回
#define HE_BEAT         	        0x31    //站点心跳
#define HE_BEAT_BACK                    0x32    //站点心跳返回
#define CA_RENT             	  	0x41    //刷卡租车
#define CA_RENT_BACK        	  	0x42    //刷卡租车返回
#define CA_RENT_AFFIRM      		0x43    //刷卡租车确认
#define CA_RENT_AFFIRM_BACK      	0x44    //刷卡租车确认返回
#define BI_AUTHORIZATION      		0x51    //车辆入桩授权
#define BI_AUTHORIZATION_BACK 		0x52    //车辆入桩授权返回
#define CH_KNEE			   	0x61    //查询桩信息
#define CH_KNEE_BACK 			0x62    //查询桩信息返回
#define BI_OUT			   	0x71    //刷卡授权出桩
#define BI_OUT_BACK 			0x72    //刷卡授权出桩返回
#define BOOTLOAD_IN                     0x81    //虚拟服务器发送进入bootload指令
#define BOOTLOAD_OUT                    0x82    //虚拟服务器发送退出bootload指令
#define ERROR_APP                       0x91    //应用程序错误

#define PACKAGE_state                   0x00    //包态

void TCP_KPD(int sockfd);
void TCP_SW(int sockfd);

#endif
