/*USART_to_TCP*/
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <pthread.h>
#include <linux/types.h>
#include "Terminal.h"

extern int Red_light;

extern RUNNING_STATE isRunning;
extern INIT_AT ATState;
extern struct Terminal_Info Terminal;

int isDisconnected = -1;
int error_time = 0;
int USART_fd;

void ResetAT()
{
    system("echo 0 > /sys/class/gpio/gpio139/value");
    sleep(1);
    system("echo 1 > /sys/class/gpio/gpio139/value");
    sleep(2);
}

void UsartReceive(char *str,int sec)
{
    tcflush(USART_fd,TCIOFLUSH);
    if(write(USART_fd,str,strlen(str))>0)printf("Write:%s",str);
    int j;
    for(j=0;j<sec;j++)
    {
        sleep(1);
        t_daemon("netfile");
    }
    char buf[1024] = {0};
    fd_set p_rd;
    FD_ZERO(&p_rd);
    FD_SET(USART_fd,&p_rd);
    struct timeval tv={1,0};
    if(FD_ISSET(USART_fd,&p_rd) && select(USART_fd + 1,&p_rd,NULL,NULL,&tv) > 0)
    {
        if(read(USART_fd, buf, 1024)>0)
        {
            printf("buf is %s\r\n",buf);
        }
    }
    t_daemon("netfile");
}

int USART_init(void)
{
    struct termios opt;
    int stty_fd = open("/dev/ttyS3",O_RDWR);
    if (-1==stty_fd)
    {
        perror("open device");
        return stty_fd;
    }
    tcgetattr(stty_fd,&opt);
    tcflush(stty_fd,TCIOFLUSH);
    cfsetispeed(&opt,B115200);
    cfsetospeed(&opt,B115200);
    opt.c_lflag |= ICANON;
    opt.c_lflag &= ~ISIG;
    opt.c_oflag &= ~ONLCR;
    opt.c_lflag &= ~ECHO;
    opt.c_cflag &= ~CSIZE;
    opt.c_cflag |= CS8;
    opt.c_cflag &= ~PARENB;
    opt.c_iflag &= ~INPCK;
    opt.c_cflag &= ~CSTOPB;
    opt.c_cc[VTIME] = 150;
    opt.c_cc[VMIN] = 0;
    opt.c_lflag &= ~ICANON;
    opt.c_iflag &= ~(IXON|ICRNL|BRKINT|ISTRIP);

    if(0!=tcsetattr(stty_fd,TCSANOW,&opt))
    {
        perror("set baudrate");
        return -1;
    }
    tcflush(stty_fd,TCIOFLUSH);
    return stty_fd;
}

// 初始化AT模块
void init_AT()
{
    ResetAT();
    char *str[] = {
        "+++",
        "\r\n",
        "AT+CWMODE=1\r\n",
        "AT+CWJAP_CUR=\"ESP999999\",\"11119999\"\r\n",
        "AT+CIPSTART=\"TCP\",\"115.159.102.193\",58000\r\n",
        "AT+CIPSTATUS\r\n",
        "AT+CIPMODE=1\r\n",
        "AT+CIPSEND\r\n"
    };
    int i;
    for(i=0;i<8;)
    {
        switch(i)
        {
        case 3:UsartReceive(str[i],12);i++;break;
        case 4:UsartReceive(str[i],5);i++;break;
        case 5:
        {
            tcflush(USART_fd,TCIOFLUSH);
            if(write(USART_fd,str[i],strlen(str[i]))>0)printf("Write:%s",str[i]);
            sleep(1);
            int status = 0;
            char buf[128] = {0};
            fd_set p_rd;
            FD_ZERO(&p_rd);
            FD_SET(USART_fd,&p_rd);
            struct timeval tv={1,0};
            if(FD_ISSET(USART_fd,&p_rd) && select(USART_fd + 1,&p_rd,NULL,NULL,&tv) > 0)
            {
                if(read(USART_fd, buf, 128)>0)
                {
                    printf("buf is %s",buf);
                    char *b = strstr(buf,"STATUS:");
                    if(b!=NULL && *(b-1)!='P')
                    {
                        switch(b[7])
                        {
                        case '2':
                            printf("状态2已连接到wifi,重新连接TCP到后台\r\n");
                            i=4;
                            error_time++;
                            break;
                        case '3':
                            printf("状态3已连接到服务器,开启透传\r\n");
                            status = 1;
                            i++;
                            break;
                        default:
                            printf("错误编号%c,重启AT模块\r\n",b[7]);
                            i=2;
                            error_time++;
                            break;
                        }
                    }
                    else error_time++;
                }
            }
            else
            {
                i=0;
                error_time++;
            }
            printf("error_time is %d\r\n",error_time);
            t_daemon("netfile");

            //记录错误次数 如果错误次数超过3则复位模块
            if(error_time >= 2)
            {
                isRunning = PROGRAM_VIRTUAL_SERVER;     //跳转到虚拟服务器
                return;
            }
            Red_light = status?LIGHT_OFF:LIGHT_BLINK;
            break;
        }
        default:UsartReceive(str[i],1);i++;break;
        }
    }
    printf("USART Init Completed\r\n");
    isDisconnected = 0;
    ATState = NOT_TO_INIT;
}
