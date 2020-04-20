#ifndef _UDP_TERMINAL_H_
#define _UDP_TERMINAL_H_

#define UDP_REMOTE_IP "128.0.1.1"
#define UDP_REMOTE_PORT 8080
#define UDP_LOCAL_IP "128.0.1.130"
#define UDP_LOCAL_PORT 8090

void* UDP_thread(void *arg);
int UdpSocketInit(const char *localIP, const int localPort, const char *remoteIP, const int remotePort);

#endif
