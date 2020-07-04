#ifndef _TYPE_DEFINE_H_
#define _TYPE_DEFINE_H_

#define bool int    //linux C中没有bool类型
#define false 0     //linux C中没有bool类型
#define true  1     //linux C中没有bool类型

struct TransMessage {
    unsigned int length;
    void *data;
};

struct Terminal_MsgSt {
    long MsgType;               //在消息队列满或者空时 =0,一直等待; IPC_NOWAIT,立即返回-1
    struct TransMessage *msg;
};

#endif