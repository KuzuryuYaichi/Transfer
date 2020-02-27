#ifndef _CHARGER_CAN_TERMINAL_H_
#define _CHARGER_CAN_TERMINAL_H_

//车桩工作状态
#define    RENT     		0x91      //借车
#define    REPAY    		0x92      //还车
#define    INQUIRE  		0x93      //查询
#define    HEART    		0x94      //心跳
#define    AFFIRM    		0x95      //借车确认
#define    CLOUD_AFFIRM         0x96      //扫码借车确认
#define    CHECK     		0x97      //后台主动查询
#define    OUT     		0x98      //刷卡授权出桩
#define    OUT_AFFIRM     	0x99      //刷卡授权出桩确认
#define    KNEE_ERROR     	0x9A      //故障
#define    OUT_TIMEOUT          0x9B      //取车超时
#define    LOW_BATTERY          0x9C      //电量过低
#define    GET_CONFIG_RESULT    0x9D      //读取配置
#define    SET_CONFIG_RESULT    0x9E      //写入配置
#define    SITE_ID_CHECK        0x9F      //站点ID查询
/***************************************/
//车桩指令编码
#define    CAN_BOOTLOAD 	0x040     //Bootload控制
#define    CAN_ERRASE   	0x080     //擦除
#define    CAN_WRITE    	0x0C0     //写入
#define    CAN_MCU_VSON         0x140     //主控版本号
#define    CAN_DISP_VSON        0x180     //显控版本号
#define    CAN_PRODUCT_TIME	0x1C0     //生产日期
#define    CAN_CARRIER_VSON     0x200     //载波版本号
#define    CAN_LOGO_VSON        0x240     //LOGO版本号
#define    CAN_PCRD_VSON        0x280     //断电复位器版本号
#define    CAN_CARD     	0x300     //卡号
#define    CAN_KNEE     	0x340     //桩号
#define    CAN_BIKE     	0x380     //车号
#define    CAN_CHARGE   	0x3C0     //充电
#define    CAN_MAL      	0x400     //故障
#define    CAN_HEART    	0x440     //心跳
#define    CAN_CHECK    	0x480     //查询
#define    CAN_SITE_ID    	0x4C0     //获取站点号
#define    CAN_BOARD    	0x600     //广播(暂时用于亮度调节)
#define    CAN_ORDER    	0x640     //指令
#define    CAN_INFORM    	0x680     //账户信息
#define    CAN_EXPENSE    	0x6C0     //消费信息
/***************************************/

int CanSendData(int sockfd);
int RecvCanData(int sockfd);

#endif
