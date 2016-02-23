#pragma once

#include <WinSock2.h>
#pragma comment(lib,"WS2_32")

class CInitSock
{
public:
	CInitSock(BYTE minorVer = 2, BYTE majorVER = 2)
	{
		//³õÊ¼»¯WS2_32.dll
		WSADATA wsaData;
		WORD sockVersion = MAKEWORD(minorVer, majorVER);
		if (::WSAStartup(sockVersion, &wsaData) != 0)
			exit(0);
	}
	~CInitSock()
	{
		::WSACleanup();
	}
};

