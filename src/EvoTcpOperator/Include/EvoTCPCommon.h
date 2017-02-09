#ifndef EVOTCPCOMMON_H
#define EVOTCPCOMMON_H

#ifdef _WIN32

#include <windows.h>
#include <process.h>

typedef unsigned int u_int32_t;
typedef int socklen_t;

#else

#include <pthread.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <linux/tcp.h>
#include <sys/ioctl.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdint.h>

typedef u_int32_t           DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef int                 SOCKET;
typedef void*               PVOID;

extern int errno;

#define WINAPI
#define CRITICAL_SECTION                pthread_mutex_t
#define InitializeCriticalSection(cs)   {pthread_mutexattr_t mutexattr; pthread_mutexattr_init(&mutexattr); pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP); pthread_mutex_init(cs,&mutexattr); pthread_mutexattr_destroy(&mutexattr);}
#define EnterCriticalSection            pthread_mutex_lock
#define LeaveCriticalSection            pthread_mutex_unlock
#define DeleteCriticalSection           pthread_mutex_destroy
#define closesocket                     close
#define Sleep(msec)                     usleep((msec)*1000)
#define INVALID_SOCKET                  (SOCKET)(~0)
#define SOCKET_ERROR                    (-1)
#define TerminateThread(handle,other)   pthread_cancel(handle)
#define WSAGetLastError()               errno
#define GetLastError()                  errno
#define WSAEWOULDBLOCK                  EAGAIN
#define TRUE                            1
#define FALSE                           0
#define _strdup                         strdup
#define ioctlsocket                     ioctl

#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "../Include/EvoTCPMessage.h"
#include "../Include/TCPTaskList.h"
#include "../Include/EvoNetPortState.h"
#include "../Include/TCPConnectionList.h"
#include "../Include/EvoTCPNetworkEvent.h"
#include "../Include/TCPTaskResolve.h"
#include "../Include/EvoTCPSendThread.h"
#include "../Include/EvoTCPRecvThread.h"
#include "../Include/EvoNetMonitor.h"
#include "../Include/EvoTCPOperator.h"



#endif