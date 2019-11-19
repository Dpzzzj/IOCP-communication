#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h> 
#define BUF_SIZE 1000
#define NAME_SIZE 20
 
#pragma comment(lib, "ws2_32.lib")  //���� ws2_32.dll  
 
unsigned WINAPI SendMsg(void * arg);//������Ϣ����
unsigned WINAPI RecvMsg(void * arg);//������Ϣ����
void ErrorHandling(char * msg);//���󷵻غ���
 
int haveread = 0;
char NAME[50];//[����]
char ANAME[50];
char msg[BUF_SIZE];//��Ϣ
 
int main(int argc, char *argv[])
{
 
    printf("������������");
    scanf("%s", NAME);
    WSADATA wsaData;
    SOCKET hSock;
    SOCKADDR_IN servAdr;
    HANDLE hSndThread, hRcvThread;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSA����ʧ�ܡ�����");
	}
 
    hSock = socket(PF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == hSock)
	{
		printf("���󣬽����׽���ʧ��...");
	}
	else
	{
		printf("�����׽��ֳɹ�\n");
	}
    memset(&servAdr, 0, sizeof(servAdr));
    servAdr.sin_family = AF_INET;
    servAdr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    servAdr.sin_port = htons(6666);
 
	if (connect(hSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
	{
		printf("����ʧ�ܡ�����");
		exit;
	}

 
    int resultsend;
    puts("Welcome to joining our chatting room!\n");
    sprintf(ANAME, "[%s]", NAME);
 
    hSndThread =
        (HANDLE)_beginthreadex(NULL, 0, SendMsg, (void*)&hSock, 0, NULL);//д�߳�
    hRcvThread =
        (HANDLE)_beginthreadex(NULL, 0, RecvMsg, (void*)&hSock, 0, NULL);//���߳�
 
    WaitForSingleObject(hSndThread, INFINITE);//�ȴ��߳̽���
    WaitForSingleObject(hRcvThread, INFINITE);
    closesocket(hSock);
    WSACleanup();
    system("pause");
    return 0;
}
 
unsigned WINAPI SendMsg(void * arg)   // send thread main
{
    SOCKET sock = *((SOCKET*)arg);
    char name_msg[NAME_SIZE + BUF_SIZE];
    char padd[2];
    fgets(padd, 2, stdin);//�����'\n'
    printf("\n send message:");
    while (1)
    {
        {
            fgets(msg, BUF_SIZE, stdin);
            if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
            {
                closesocket(sock);
                exit(0);
            }
            sprintf(name_msg, "[%s] %s", NAME, msg);
            char numofmsg = strlen(name_msg) + '0';
            char newmsg[100]; newmsg[0] = numofmsg; newmsg[1] = 0;//��һ���ַ���ʾ��Ϣ�ĳ���
            strcat(newmsg, name_msg);
            int result = send(sock, newmsg, strlen(newmsg), 0);
            if (result == -1)return -1;//���ʹ���
        }
    }
    return NULL;
}
 
unsigned WINAPI RecvMsg(void * arg)  // read thread main
{
    SOCKET sock = *((SOCKET*)arg);
    char name_msg[NAME_SIZE + BUF_SIZE];
    int str_len = 0;
    while (1)
    {
        {
            char lyfstr[1000] = { 0 };
            int totalnum = 0;
            str_len = recv(sock, name_msg, 1, 0);//��ȡ��һ���ַ�����ȡ��Ϣ�ĳ���
            if (str_len == -1)//��ȡ����
            {
                printf("return -1\n");
                return -1;
            }
            if (str_len == 0)//��ȡ����
            {
                printf("return 0\n");
                return 0;//��ȡ����
            }
            totalnum = name_msg[0] - '0';
            int count = 0;
 
            do
            {
                str_len = recv(sock, name_msg, 1, 0);
 
                name_msg[str_len] = 0;
 
                if (str_len == -1)//��ȡ����
                {
                    printf("return -1\n");
                    return -1;
                }
                if (str_len == 0)
                {
                    printf("return 0\n");
                    return 0;//��ȡ����
                }
                strcat(lyfstr, name_msg);
                count = str_len + count;
 
            } while (count < totalnum);
 
            lyfstr[count] = '\0';
            printf("\n");
            strcat(lyfstr, "\n");
            fputs(lyfstr, stdout);
            printf(" send message:");
            fflush(stdout);
            memset(name_msg, 0, sizeof(char));
        }
    }
    return NULL;
}
 
void ErrorHandling(char * msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}