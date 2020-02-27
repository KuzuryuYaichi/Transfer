#ifndef _TERMINAL_TERMINAL_H_
#define _TERMINAL_TERMINAL_H_

#define EQUIP_NUM_MAX 64            //最大充电桩数

#define LIGHT_OFF 0
#define LIGHT_ON 1
#define LIGHT_BLINK 2

extern int CanRecv_qid;
extern int UpdataRecv_qid;

/***********************************************************/
//桩体信息
struct Knee_struct {                //桩体数据结构体
    __u64 knee_ID;                  //充电桩ID号(10进制)
    __u8 card_type;                 //卡的类型  0x00 用户卡，0x03 管理员卡
    __u8 card_ID[4];
    __u8 bike_ID[7];
    __u8 mal_infor[5];              //故障信息
    __u8 charge_infor[5];           //充电信息
    int heart_count;                //心跳数量统计
    __u8 recv_count;                //用于判断借车/换车，bit0:接收到0x300指令，bit1:接收到0x340指令，bit2:接收到0x380指令
    __u8 function;                  //功能位：0x8查询，0x4借车
    __u64 additional;               //附加值
    __u64 open_type;                //开锁方式
    __u64 additional_value;         //请求包附加值
    pthread_mutex_t mt;             //线程同步互斥对象
};
/***********************************************************/
struct UpData_struct {              //程序升级UDP数据结构体
    __u8 equip_name;                //程序类型
    char data[256];                 //接收数据(286字节)
    __u32 program_addr;             //程序写入起始地址
    __u32 program_size;             //程序总量
};
/***********************************************************/
struct UdpScan_struct {             //扫描返回数据
   __u32 MH;                        //主控硬件版本号
   __u32 MS;                        //主控软件版本号
   __u32 DH;                        //显控硬件版本号
   __u32 DS;                        //显控软件版本号
   __u32 CH;                        //载波硬件版本号
   __u32 CS;                        //载波软件版本号
   __u32 LH;                        //字库硬件版本号
   __u32 LS;                        //字库软件版本号
   __u32 PH;                        //断电复位器硬件版本号
   __u32 PS;                        //断电复位器软件版本号
   pthread_mutex_t mt;              //线程同步互斥对象
};
/********************服务器返回数据************************/
struct Net_struct {
    __u16 function_type;            //指令类型：刷卡借车、还车、扫码借车
    __u64 site_ID;                  //站点ID
    __u64 knee_ID;                  //车桩ID
    int result;                     //执行结果
    int account;                    //账户余额
    char *account_str;              //字符串类型账户余额
    int consumption;                //消费金额
    char *rent_begin;               //开始租车时间
    int time_length;                //租车时长
    pthread_mutex_t mt;             //线程同步互斥对象
};
/*********************消息结构体********************/
struct Terminal_MsgSt {
    long int MsgType;               //在消息队列满或者空时 =0,一直等待; IPC_NOWAIT,立即返回-1
    char MsgBuff[2];                //前两字节用于存放功能码，第三字节存放桩位号
};
/*******************查询桩信息结构体*****************/
struct CheckKnee_Struct {
    __u8 count;                     //查询计数
    __u8 check_flag;                //查询到的信息标志
    __u8 isUser;                    //是否为用户
    __u64 bike_id;                  //当前车号
};
/******************数据终端自身信息信息结构体******************/
struct Terminal_Info {
    int Customer_ID;                //客户编号
    char Hardware_version[10];      //硬件版本
    char Software_version[10];      //软件版本
    __u64 SITE_ID;                  //站点ID
    char SALT_site[10];             //站点盐值
    char SALT_password[32];         //站点密码
    char Server[32];                //站点域名
    char Server_IP[32];             //服务器IP
    int Server_Port;                //服务器PORT
    char Wifi_password[32];         //Wifi密码
    char Wifi_name[32];             //Wifi名
    char TOKEN_site[128];           //站点通讯token
    __u64 ConnectCount;             //重连次数
    __u64 DisConnectCount;          //断线次数
    __u64 ResetCount;               //重启次数
    int BackStage_Sel;              //连接后台
};
/******************程序跳转状态********************
在进入Bootloader时需要判断是否读取APPFLAG 要不要初始化AT模块
在进入虚拟服务器时需要判断要不要初始化AT模块
在进入真实服务器时不需要判断
上电直接进入Bootloader 此时需要判断APPFLAG 连接虚拟服务器需要初始化AT模块
虚拟服务器指令进入Bootloader 此时不需要判断APPFLAG 连接虚拟服务器不需要初始化AT模块
上电复位进入Bootloader后进入虚拟服务器 此时需要判断是否连接过虚拟服务器 需要初始化AT模块
虚拟服务器指令进入Bootloader后进入虚拟服务器 此时需要判断APPFLAG 连接虚拟服务器需要初始化AT模块
虚拟服务器进入真实服务器 需要初始化AT模块
************************************************/
typedef enum RUNNING_STATE{
    POWER_OFF = '0',
    PROGRAM_BOOTLOADER,
    PROGRAM_VIRTUAL_SERVER,
    PROGRAM_ACTUAL_SERVER
}RUNNING_STATE;
/***********************定义程序间跳转的传参格式****************************
第一个参数 从哪里跳转(空 Bootloader默认上电 VirutalServer默认Bootloader RealServer默认VirtualServer)
第二个参数 到哪里跳转(空 不做处理)
第三个参数 是否需要初始化AT模块(空 Bootloader 默认需要)
第四个参数 如果已经初始化了AT要传递SITE_COMMON_TOKEN
**********************************************************************/
typedef enum INIT_AT{
    NOT_TO_INIT = '0',
    NEED_TO_INIT
}INIT_AT;

#endif
