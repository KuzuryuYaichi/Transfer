.SUFFIXES : .x .o .c .s

CC = aarch64-linux-gnu-gcc
STRIP = aarch64-linux-gnu-strip

TARGET = t2
SRCS = Terminal.c Monitor.c TCPServer.c UDP.c EpollEvent.c
LIBS = -lpthread -lc -lgcc -lm

all:
	$(CC) $(SRCS) -o $(TARGET) $(LIBS) -g
#	$(STRIP) $(TARGET)

clean:
	rm -f *.o
	rm -f *.x
	rm -f *.flat
	rm -f *.map
	rm -f temp
	rm -f *.img
	rm -f $(TARGET)
	rm -f *.gdb
