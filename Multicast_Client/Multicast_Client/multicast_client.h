#pragma once

#include <iostream>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include "audio_api.h"

#pragma comment(lib, "ws2_32")

bool init_winsock(WSADATA *stWSAData);
bool get_datagram_socket(SOCKET *hSocket);
bool bind_socket(SOCKADDR_IN *stLclAddr, SOCKET *hSocket, u_short nPort);
bool set_socket_option_reuseaddr(SOCKET *hSocket);
bool join_multicast_group(struct ip_mreq *stMreq, SOCKET *hSocket, char *achMCAddr);
bool leave_multicast_group(struct ip_mreq *stMreq, SOCKET *hSocket, char *achMCAddr);
DWORD WINAPI receive_data(LPVOID lp);
void CALLBACK completion_routine(DWORD Error, DWORD BytesTransferred, LPWSAOVERLAPPED Overlapped, DWORD InFlags);
void CALLBACK FileStream_ReceiveRoutine(DWORD Error, DWORD BytesTransferred, LPWSAOVERLAPPED Overlapped, DWORD InFlags);
bool play_audio();

#define BUFSIZE			8912
#define MAXADDRSTR		16
#define TIMECAST_ADDR   "234.5.6.7"
#define TIMECAST_PORT   8910




typedef struct _SOCKET_INFORMATION {
	OVERLAPPED overlapped;
	SOCKET *hSocket;
	char buffer[BUFSIZE];
	WSABUF DataBuf;
	DWORD BytesSEND;
	DWORD BytesRECV;
	SOCKADDR_IN *stSrcAddr;
} SOCKET_INFORMATION, *LPSOCKET_INFORMATION;