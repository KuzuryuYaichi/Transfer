#ifndef _TERMINAL_TERMINAL_H_
#define _TERMINAL_TERMINAL_H_

typedef enum RUNNING_STATE{
    POWER_OFF = '0',
    PROGRAM_BOOTLOADER,
    PROGRAM_VIRTUAL_SERVER,
    PROGRAM_ACTUAL_SERVER
}RUNNING_STATE;

typedef enum INIT_AT{
    NOT_TO_INIT = '0',
    NEED_TO_INIT
}INIT_AT;

#endif