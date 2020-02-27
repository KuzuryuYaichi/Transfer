.SUFFIXES : .x .o .c .s

CC := arm-linux-gcc
STRIP := arm-linux-strip

TARGET = BOOT
SRCS := Terminal.c can.c GPIO.c Charger_can.c TCP_SW.c TCP_KPD.c CRC.c CJSON.c MD5.c Base64.c USART.c CommonFunc.c
LIBS = -lpthread -lc -lgcc -lm

all:
	$(CC) $(SRCS) -o $(TARGET) $(LIBS)
	$(STRIP) $(TARGET)

clean:
	rm -f *.o
	rm -f *.x
	rm -f *.flat
	rm -f *.map
	rm -f temp
	rm -f *.img
	rm -f $(TARGET)
	rm -f *.gdb


