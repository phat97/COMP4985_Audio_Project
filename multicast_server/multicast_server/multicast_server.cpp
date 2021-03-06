// multicast_server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "pch.h"
#include "multicast_server.h"


int main() {
	SOCKADDR_IN stLclAddr, stDstAddr;
	struct ip_mreq stMreq;        /* Multicast interface structure */
	SOCKET hSocket;
	WSADATA stWSAData;
	WSAEVENT sendEvent;
	SOCKET_INFORMATION si;
	HANDLE broadcast_thread;
	HANDLE audio_buffering_thread;

	char achMCAddr[MAXADDRSTR] = TIMECAST_ADDR;
	u_short nPort = TIMECAST_PORT;
	u_long  lTTL = TIMECAST_TTL;

	/*if ((audio_buffering_thread = CreateThread(NULL, 0, store_audio_data, &si, 0, 0)) == NULL) {
		printf("CreateThread failed with error %d\n", GetLastError());
		exit(1);
	}*/


	if (!init_winsock(&stWSAData)) {
		exit(1);
	}

	if (!get_datagram_socket(&hSocket)) {
		WSACleanup();
		exit(1);
	}

	if (!bind_socket(&stLclAddr, &hSocket, nPort)) {
		closesocket(hSocket);
		WSACleanup();
		exit(1);
	}

	printf("Server Started\n");

	if (!join_multicast_group(&stMreq, &hSocket, achMCAddr)) {
		closesocket(hSocket);
		WSACleanup();
		exit(1);
	}

	if (!set_ip_ttl(&hSocket, lTTL)) {
		closesocket(hSocket);
		WSACleanup();
		exit(1);
	}

	if (!disable_loopback(&hSocket)) {
		closesocket(hSocket);
		WSACleanup();
		exit(1);
	}

	printf("Multicast prepared\n");

	stDstAddr.sin_family = AF_INET;
	stDstAddr.sin_addr.s_addr = inet_addr(achMCAddr);
	stDstAddr.sin_port = htons(nPort);

	/*if ((sendEvent = WSACreateEvent()) == WSA_INVALID_EVENT) {
		printf("WSACreateEvent() failed with error %d\n", WSAGetLastError());
		closesocket(hSocket);
		WSACleanup();
		exit(1);
	}*/

	si.hSocket = &hSocket;
	si.stDstAddr = &stDstAddr;

	printf("Main() hSocket: %d\n", hSocket);


	if ((broadcast_thread = CreateThread(NULL, 0, broadcast_data, &si, 0, 0)) == NULL) {
		printf("CreateThread failed with error %d\n", GetLastError());
		closesocket(hSocket);
		WSACleanup();
		exit(1);
	}
	printf("Waiting\n");
	WaitForSingleObject(broadcast_thread, INFINITE);
	//WaitForSingleObject(audio_buffering_thread, INFINITE);

	printf("Closing\n");
	closesocket(hSocket);
	WSACleanup();
	return 0;
}




bool init_winsock(WSADATA *stWSAData) {
	int nRet;
	if (nRet = WSAStartup(0x0202, &(*stWSAData))) {
		printf("WSAStartup failed: %d\r\n", nRet);
		return false;
	}
	return true;
}

bool get_datagram_socket(SOCKET *hSocket) {
	*hSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (*hSocket == INVALID_SOCKET) {
		printf("socket() failed, Err: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

bool bind_socket(SOCKADDR_IN *stLclAddr, SOCKET *hSocket, u_short nPort) {
	stLclAddr->sin_family = AF_INET;
	stLclAddr->sin_addr.s_addr = htonl(INADDR_ANY); /* any interface */
	stLclAddr->sin_port = 0;                 /* any port */
	if (bind(*hSocket, (struct sockaddr*) &(*stLclAddr), sizeof(*stLclAddr)) == SOCKET_ERROR) {
		printf("bind() port: %d failed, Err: %d\n", nPort, WSAGetLastError());
		return false;
	}
	return true;
}

bool join_multicast_group(struct ip_mreq *stMreq, SOCKET *hSocket, char *achMCAddr) {
	stMreq->imr_multiaddr.s_addr = inet_addr(achMCAddr);
	stMreq->imr_interface.s_addr = INADDR_ANY;
	if (setsockopt(*hSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&(*stMreq), sizeof(*stMreq)) == SOCKET_ERROR) {
		printf("setsockopt() IP_ADD_MEMBERSHIP address %s failed, Err: %d\n", achMCAddr, WSAGetLastError());
		return false;
	}
	return true;
}

bool set_ip_ttl(SOCKET *hSocket, u_long  lTTL) {
	if (setsockopt(*hSocket, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&lTTL, sizeof(lTTL)) == SOCKET_ERROR) {
		printf("setsockopt() IP_MULTICAST_TTL failed, Err: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

bool disable_loopback(SOCKET *hSocket) {
	BOOL fFlag = false;
	if (setsockopt(*hSocket, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&fFlag, sizeof(fFlag)) == SOCKET_ERROR) {
		printf("setsockopt() IP_MULTICAST_LOOP failed, Err: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

DWORD WINAPI broadcast_data(LPVOID lp) {
	SOCKET_INFORMATION si = *(LPSOCKET_INFORMATION)lp;
	char *file_stream_buf;
	int bytes_read;
	FILE* fp;
	DWORD SendBytes;

	if (!fopen_s(&fp, "test.wav", "rb") == 0) {
		printf("Open file error\n");
		exit(1);
	}
	printf("File open\n");
	file_stream_buf = (char*)malloc(BUFSIZE);
	si.DataBuf.buf = si.buffer;

	while (!feof(fp)) {

		memset(file_stream_buf, 0, BUFSIZE);
		bytes_read = fread(file_stream_buf, 1, BUFSIZE, fp);

		memcpy(si.DataBuf.buf, file_stream_buf, bytes_read);
		si.DataBuf.len = (ULONG)bytes_read;
		printf("Sending\n");
		//Sleep(20);
		if (WSASendTo(*si.hSocket, &si.DataBuf, 1, &SendBytes, 0, (struct sockaddr*)si.stDstAddr, sizeof(*si.stDstAddr), 0, 0) == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				printf("Error\n");
				return FALSE;
			}
		}
	}
	
	return TRUE;
}

DWORD WINAPI store_audio_data(LPVOID lp) {

	printf("Reading into buffer\n");

	return TRUE;
}