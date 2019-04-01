#pragma once

#include <iostream>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32")

bool init_winsock(WSADATA *stWSAData);
bool get_datagram_socket(SOCKET *hSocket);
bool bind_socket(SOCKADDR_IN *stLclAddr, SOCKET *hSocket, u_short nPort);
bool join_multicast_group(struct ip_mreq *stMreq, SOCKET *hSocket, char *achMCAddr);
bool set_ip_ttl(SOCKET *hSocket, u_long  lTTL);
bool disable_loopback(SOCKET *hSocket);
DWORD WINAPI broadcast_data(LPVOID lp);
DWORD WINAPI store_audio_data(LPVOID lp);

#define BUFSIZE			8912
#define MAXADDRSTR		16
#define TIMECAST_ADDR   "234.5.6.7"
#define TIMECAST_PORT   8910
#define TIMECAST_TTL    2


typedef struct _SOCKET_INFORMATION {
	OVERLAPPED overlapped;
	SOCKET *hSocket;
	char buffer[BUFSIZE];
	WSABUF DataBuf;
	DWORD BytesSEND;
	DWORD BytesRECV;
	SOCKADDR_IN *stDstAddr;
} SOCKET_INFORMATION, *LPSOCKET_INFORMATION;

