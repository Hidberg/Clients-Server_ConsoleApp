// Sokets_Task.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <winsock2.h>
#include <locale.h>
#include <ws2tcpip.h>

#pragma comment(lib,"ws2_32.lib") //Winsock Library

typedef enum { textMsg, fileMsg } msg_type;

int saveReceivedFile(SOCKET s, char *msgFromClient)
{
	char *fname = (char*)calloc(500, sizeof(char));
	FILE *fileForSaveData;
	puts("Куда сохранить пересылаемый файл? Введите путь.");
	gets(fname);
	while (fopen(fname, "wb") == NULL)
	{
		puts("Путь введен неверно или туда невозможно сохранить файл, введите ещё раз.");
		free(fname);
		fname = (char*)calloc(500, sizeof(char));
		gets(fname);
	}
	fileForSaveData = fopen(fname, "wb");
	free(fname);
	char *receivedDataSize_str = (char*)calloc(4, sizeof(char));
	for (int i = 1; i < 5; ++i)
	{
		receivedDataSize_str[i - 1] = msgFromClient[i];
	}
	int receivedDataSize = *((int*)receivedDataSize_str);
	free(receivedDataSize_str);
	int temp;
	char *fileData = (char*)malloc(receivedDataSize*sizeof(char));
	if ((temp = recv(s, fileData, receivedDataSize, 0)) != SOCKET_ERROR && temp != 0)
	{
		fwrite(fileData, sizeof(char), receivedDataSize, fileForSaveData);
		puts("Файл успешно сохранен.");
	}
	else
	{
		puts("Файл не сохранен!");
	}
	free(fileData);
	fclose(fileForSaveData);
	return 0;
}

int serverWork()
{
	WSADATA wsa;
	SOCKET s, new_socket;
	struct sockaddr_in server, client;
	int c;
	char *msgFromClient;
	int recv_size;

	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		return 1;
	}
	printf("Initialised.\n");

	//Create a socket
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d", WSAGetLastError());
	}
	printf("Socket created.\n");

	//Prepare the sockaddr_in structure
	server.sin_addr.s_addr = inet_addr("192.168.1.38");
	server.sin_family = AF_INET;
	server.sin_port = htons(58888);

	//Bind
	if (bind(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
	{
		printf("Bind failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	puts("Bind done");

	//Listen to incoming connections
	listen(s, 1);

	//Accept and incoming connection
	puts("Waiting for incoming connections...");

	c = sizeof(struct sockaddr_in);

	while ((new_socket = accept(s, (struct sockaddr *)&client, &c)) != INVALID_SOCKET)
	{
		puts("Connection accepted");
		while (true)
		{
			if (send(new_socket, "check", 5, 0) < 0)
			{
				puts("Connection lost.\nWaiting for incoming connections...");
				break;
			}
			msgFromClient = (char*)malloc(5 * sizeof(char));
			if ((recv_size = recv(new_socket, msgFromClient, 5, 0)) != SOCKET_ERROR)
			{
				if ((msgFromClient[0]) == fileMsg)
				{
					saveReceivedFile(new_socket, msgFromClient); // msgFromClient parametr for sizeDataFile
				}
				else if ((msgFromClient[0]) == textMsg)
				{
					int receivedDataSize = *((int*)(msgFromClient+1));
					char *textMsg = (char*)malloc((receivedDataSize+1)*sizeof(char));
					int temp = 0;
					temp = recv(new_socket, textMsg, receivedDataSize, 0);
					if (temp != SOCKET_ERROR && temp != 0)
					{
						puts("Message received:");
						textMsg[temp] = '\0';
						puts(textMsg);
					}
					free(textMsg);
				}
			}
			free(msgFromClient);
		}
	}

	closesocket(s);
	WSACleanup();
	return 0;
}

int main()
{
	setlocale(LC_ALL, "rus");
	serverWork();
	return 0;
}

