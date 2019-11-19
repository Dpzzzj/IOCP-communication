#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio> 
#include <ctime> 

using namespace std;
#pragma comment(lib,"ws2_32.lib")


#define BUF_SIZE 100
#define READ    3
#define WRITE    5

typedef struct    // socket info
{
	SOCKET hClntSock;
	SOCKADDR_IN clntAdr;
} PER_HANDLE_DATA, * LPPER_HANDLE_DATA;

typedef struct    // buffer info
{
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[BUF_SIZE];
	int rwMode;    // READ or WRITE 读写模式
} PER_IO_DATA, * LPPER_IO_DATA;

const char* p = "store.txt";
unsigned int  WINAPI EchoThreadMain(LPVOID CompletionPortIO);
void ErrorHandling(char* message);
SOCKET ALLCLIENT[100];
int clientcount = 0;
HANDLE hMutex;

int main(int argc, char* argv[])
{

	hMutex = CreateMutex(NULL, FALSE, NULL);//创建互斥量

	WSADATA    wsaData;
	HANDLE hComPort;
	SYSTEM_INFO sysInfo;
	LPPER_IO_DATA ioInfo;
	LPPER_HANDLE_DATA handleInfo;

	SOCKET hServSock;
	SOCKADDR_IN servAdr;
	int  i;
	DWORD recvBytes = 0, flags = 0;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);//1创建CP对象
	GetSystemInfo(&sysInfo);//获取当前系统的信息

	//for (i = 0; i < sysInfo.dwNumberOfProcessors; i++)
	for(i=0;i<1;i++)
		_beginthreadex(NULL, 0, EchoThreadMain, (LPVOID)hComPort, 0, NULL);//创建=CPU个数的线程数

	hServSock = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);//不是非阻塞套接字，但是重叠IO套接字。
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(6666);

	bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr));
	listen(hServSock, 5);

	while (1)
	{
		SOCKET hClntSock;
		SOCKADDR_IN clntAdr;
		int addrLen = sizeof(clntAdr);

		hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &addrLen);

		handleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));//和重叠IO一样
		handleInfo->hClntSock = hClntSock;

		WaitForSingleObject(hMutex, INFINITE);//线程同步

		ALLCLIENT[clientcount++] = hClntSock;//存入套接字队列

		ReleaseMutex(hMutex);

		memcpy(&(handleInfo->clntAdr), &clntAdr, addrLen);

		CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)handleInfo, 0);//连接套接字和CP对象
																				//已完成信息将写入CP对象
		ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));//存储接收到的信息
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));

	
		ioInfo->wsaBuf.len = BUF_SIZE;
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		ioInfo->rwMode = READ;//读写模式
	
		WSARecv(handleInfo->hClntSock, &(ioInfo->wsaBuf),//非阻塞模式
			1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);
		
	}
	CloseHandle(hMutex);//销毁互斥量
	return 0;
}

unsigned int WINAPI EchoThreadMain(LPVOID pComPort)//线程的执行
{ 
	HANDLE hComPort = (HANDLE)pComPort;
	SOCKET sock;
	DWORD bytesTrans;
	LPPER_HANDLE_DATA handleInfo;
	LPPER_IO_DATA ioInfo;
	DWORD flags = 0;

	while (1)//大循环
	{
		GetQueuedCompletionStatus(hComPort, &bytesTrans,//确认“已完成”的I/O！！
			(LPDWORD)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);//INFINITE使用时，程序将阻塞，直到已完成的I/O信息写入CP对象
		sock = handleInfo->hClntSock;//客户端套接字

		if (ioInfo->rwMode == READ)//读写模式（此时缓冲区有数据）
		{
			
			puts("message received!\n");
			if (bytesTrans > 0)
			{
				FILE* p;
				p = fopen("1.txt", "a");
				fseek(p, -20, 0);
				fwrite(ioInfo->buffer, bytesTrans, 1, p);
				fclose(p);
				
			}

			if (bytesTrans == 0)    // 连接结束
			{
				WaitForSingleObject(hMutex, INFINITE);//线程同步

				closesocket(sock);
				int i = 0;
				while (ALLCLIENT[i] == sock) { i++; }
				ALLCLIENT[i] = 0;//断开置0

				ReleaseMutex(hMutex);

				free(handleInfo); free(ioInfo);
				continue;
			}
			int i = 0;

			for (; i < clientcount; i++)
			{
				if (ALLCLIENT[i] != 0)//判断是否为已连接的套接字
				{
					if (ALLCLIENT[i] != sock)
					{
						LPPER_IO_DATA newioInfo;
						newioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));//动态分配内存
						memset(&(newioInfo->overlapped), 0, sizeof(OVERLAPPED));
						strcpy(newioInfo->buffer, ioInfo->buffer);//重新构建新的内存，防止多次释放free
						newioInfo->wsaBuf.buf = newioInfo->buffer;
						newioInfo->wsaBuf.len = bytesTrans;
						newioInfo->rwMode = WRITE;

						WSASend(ALLCLIENT[i], &(newioInfo->wsaBuf),//回声
							1, NULL, 0, &(newioInfo->overlapped), NULL);
					}
					else
					{
						memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
						ioInfo->wsaBuf.len = bytesTrans;
						ioInfo->rwMode = WRITE;
						WSASend (ALLCLIENT[i], &(ioInfo->wsaBuf),//回声
							1, NULL, 0, &(ioInfo->overlapped), NULL);
					}
				}
			}
			
			ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));//动态分配内存
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = BUF_SIZE;
			ioInfo->wsaBuf.buf = ioInfo->buffer;
			ioInfo->rwMode = READ;
			
			WSARecv(sock, &(ioInfo->wsaBuf),//再非阻塞式接收
					1, NULL, &flags, &(ioInfo->overlapped), NULL);
			

		}
		else
		{
			puts("message sent!");
			free(ioInfo);
		}
	}
	return 0;
}

void ErrorHandling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
