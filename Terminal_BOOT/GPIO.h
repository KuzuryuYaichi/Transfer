#ifndef _GPIO_TERMINAL_H_
#define _GPIO_TERMINAL_H_

void set_GPIOG_0_low(void);     
void set_GPIOG_0_high(void);   //告警
void set_GPIOG_1_low(void);
void set_GPIOG_1_high(void);   //通讯
void set_GPIOG_2_low(void);
void set_GPIOG_2_high(void);   //升级
void set_GPIOG_3_low(void);
void set_GPIOG_3_high(void);   //运行

void init_GPIOG139(void);

#endif
