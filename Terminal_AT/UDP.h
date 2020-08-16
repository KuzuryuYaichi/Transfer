#ifndef _UDP_TERMINAL_H_
#define _UDP_TERMINAL_H_

#include "TCPServer.h"

#define UDP_REMOTE_IP "128.0.1.1"
#define UDP_REMOTE_PORT_CC TCP_LOCAL_PORT_CC
#define UDP_REMOTE_PORT_FC TCP_LOCAL_PORT_FC

#define UDP_LOCAL_IP "128.0.82.130"
#define UDP_LOCAL_PORT 4000

void* UDP_thread(void *arg);
int UdpSocketInit(const char *localIP, const int localPort, const char *remoteIP, const int remotePort_CC, const int remotePort_FC);

#endif
