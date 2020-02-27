#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <linux/types.h>
#include <getopt.h>
#include <libgen.h>
#include <signal.h>
#include <limits.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/uio.h>

void init_GPIOG139()
{
        system("echo 139 > /sys/class/gpio/export");
        system("echo out > /sys/class/gpio/gpio139/direction");
        system("echo 0 > /sys/class/gpio/gpio139/value");
}
void set_GPIOG_0_low(void)
{
	system("echo 192 > /sys/class/gpio/export");
	system("echo out > /sys/class/gpio/gpio192/direction");	
	system("echo 0 > /sys/class/gpio/gpio192/value");
}
void set_GPIOG_0_high(void)
{
	system("echo 192 > /sys/class/gpio/export");
	system("echo out > /sys/class/gpio/gpio192/direction");	
	//system("echo 1 > /sys/class/gpio/gpio192/value");
}
void set_GPIOG_1_low(void)
{
	system("echo 193 > /sys/class/gpio/export");
	system("echo out > /sys/class/gpio/gpio193/direction");	
	system("echo 0 > /sys/class/gpio/gpio193/value");
}

void set_GPIOG_1_high(void)
{
	system("echo 193 > /sys/class/gpio/export");
	system("echo out > /sys/class/gpio/gpio193/direction");	
	//system("echo 1 > /sys/class/gpio/gpio193/value");
}
void set_GPIOG_2_low(void)
{
	system("echo 194 > /sys/class/gpio/export");
	system("echo out > /sys/class/gpio/gpio194/direction");	
	system("echo 0 > /sys/class/gpio/gpio194/value");
}

void set_GPIOG_2_high(void)
{
	system("echo 194 > /sys/class/gpio/export");
	system("echo out > /sys/class/gpio/gpio194/direction");	
	//system("echo 1 > /sys/class/gpio/gpio194/value");
}
void set_GPIOG_3_low(void)
{
	system("echo 195 > /sys/class/gpio/export");
	system("echo out > /sys/class/gpio/gpio195/direction");	
	system("echo 0 > /sys/class/gpio/gpio195/value");
}

void set_GPIOG_3_high(void)
{
	system("echo 195 > /sys/class/gpio/export");
	system("echo out > /sys/class/gpio/gpio195/direction");	
	//system("echo 1 > /sys/class/gpio/gpio195/value");
}
