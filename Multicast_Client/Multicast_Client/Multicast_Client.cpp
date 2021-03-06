#include "pch.h"
#include "multicast_client.h"


int main(int argc, char *argv[]) {
	HANDLE receive_thread;
	bool connected;
	
	connected = true;

	if ((receive_thread = CreateThread(NULL, 0, receive_data, &connected, 0, 0)) == NULL) {
		printf("CreateThread failed with error %d\n", GetLastError());
		WSACleanup();
		exit(1);
	}

	WaitForSingleObject(receive_thread, INFINITE);
	

	/* Tell WinSock we're leaving */
	WSACleanup();

	return (0);
} /* end main() */


DWORD WINAPI receive_data(LPVOID lp) {
	DWORD Flags = 0;
	DWORD RecvBytes;
	SOCKADDR_IN stLclAddr, stSrcAddr;
	struct ip_mreq stMreq;         /* Multicast interface structure */
	SOCKET hSocket;
	WSADATA stWSAData;
	char achMCAddr[MAXADDRSTR] = TIMECAST_ADDR;
	u_short nPort = TIMECAST_PORT;
	LPSOCKET_INFORMATION si;


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

	if (!set_socket_option_reuseaddr(&hSocket)) {
		closesocket(hSocket);
		WSACleanup();
		exit(1);
	}

	if (!join_multicast_group(&stMreq, &hSocket, achMCAddr)) {
		closesocket(hSocket);
		WSACleanup();
		exit(1);
	}

	if ((si = (LPSOCKET_INFORMATION)GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL) {
		printf("GlobalAlloc() failed with error %d\n", GetLastError());
		return false;
	}
	initialize_audio_device();

	ZeroMemory(&(si->overlapped), sizeof(WSAOVERLAPPED));
	si->BytesRECV = 0;
	si->BytesSEND = 0;
	si->DataBuf.len = BUFSIZE;
	si->DataBuf.buf = si->buffer;
	si->stSrcAddr = &stSrcAddr;
	si->hSocket = &hSocket;

	Flags = 0;

	int addr_size = sizeof(struct sockaddr_in);
	if (WSARecv(hSocket, &(si->DataBuf), 1, &RecvBytes, &Flags, &(si->overlapped), FileStream_ReceiveRoutine) == SOCKET_ERROR) {
		if (WSAGetLastError() != WSA_IO_PENDING) {
			printf("WSARecvFrom() failed with error %d\n", WSAGetLastError());
			return false;
		}
	}
	while (*(bool*)lp) {
		printf("%d\n", *(bool*)lp);
		printf("Before SleepEX()\n");
		SleepEx(5000, TRUE);
	}	

	

	if (!leave_multicast_group(&stMreq, &hSocket, achMCAddr)) {
		closesocket(hSocket);
		WSACleanup();
		exit(1);
	}
	/* Close the socket */
	closesocket(hSocket);
	

	return TRUE;
}

void CALLBACK completion_routine(DWORD Error, DWORD BytesTransferred, LPWSAOVERLAPPED Overlapped, DWORD InFlags) {
	DWORD RecvBytes;
	DWORD Flags;

	LPSOCKET_INFORMATION si = (LPSOCKET_INFORMATION)Overlapped;


	Flags = 0;
	ZeroMemory(&(si->overlapped), sizeof(WSAOVERLAPPED));

	si->DataBuf.len = BUFSIZE;
	si->DataBuf.buf = si->buffer;

	int addr_size = sizeof(struct sockaddr_in);
	writeAudio(si->DataBuf.buf, BytesTransferred);

	if (WSARecv(*si->hSocket, &(si->DataBuf), 1, &RecvBytes, &Flags, &(si->overlapped), completion_routine) == SOCKET_ERROR) {
		if (WSAGetLastError() != WSA_IO_PENDING) {
			printf("WSARecvFrom() failed with error %d\n", WSAGetLastError());
			return;
		}
	}
}

void CALLBACK FileStream_ReceiveRoutine(DWORD Error, DWORD BytesTransferred, LPWSAOVERLAPPED Overlapped, DWORD InFlags)
{
	DWORD RecvBytes;
	DWORD Flags;
	printf("Here I am\n");

	LPSOCKET_INFORMATION si = (LPSOCKET_INFORMATION)Overlapped;
	if (Error != 0)
	{
		printf("Error in FileStream_ReceiveRoutine\n");
	}

	OutputDebugStringA("Coming\n");
	char debug_buf[512];
	sprintf_s(debug_buf, sizeof(debug_buf), "BytesTransferred: %d\n", BytesTransferred);
	OutputDebugStringA(debug_buf);


	if (BytesTransferred != BUFSIZE) {
		return;
	}

	Flags = 0;
	ZeroMemory(&(si->overlapped), sizeof(WSAOVERLAPPED));
	
	//write_file(SI->DataBuf.buf, BytesTransferred);
	writeAudio(si->DataBuf.buf, BytesTransferred);

	si->DataBuf.buf = si->buffer;
	if (WSARecv(*si->hSocket, &(si->DataBuf), 1, &RecvBytes, &Flags, &(si->overlapped), completion_routine) == SOCKET_ERROR) {
		if (WSAGetLastError() != WSA_IO_PENDING) {
			printf("WSARecvFrom() failed with error %d\n", WSAGetLastError());
			return;
		}
	}
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
	stLclAddr->sin_port = htons(nPort);                /* any port */
	if (bind(*hSocket, (struct sockaddr*) &(*stLclAddr), sizeof(*stLclAddr)) == SOCKET_ERROR) {
		printf("bind() port: %d failed, Err: %d\n", nPort, WSAGetLastError());
		return false;
	}
	return true;
}

bool set_socket_option_reuseaddr(SOCKET *hSocket) {
	BOOL fFlag = TRUE;
	if (setsockopt(*hSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&fFlag, sizeof(fFlag)) == SOCKET_ERROR) {
		printf("setsockopt() SO_REUSEADDR failed, Err: %d\n", WSAGetLastError());
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

bool leave_multicast_group(struct ip_mreq *stMreq, SOCKET *hSocket, char *achMCAddr) {
	stMreq->imr_multiaddr.s_addr = inet_addr(achMCAddr);
	stMreq->imr_interface.s_addr = INADDR_ANY;
	if (setsockopt(*hSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&stMreq, sizeof(stMreq)) == SOCKET_ERROR) {
		printf("setsockopt() IP_DROP_MEMBERSHIP address %s failed, Err: %d\n", achMCAddr, WSAGetLastError());
		return false;
	}
	return true;
}