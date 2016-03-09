// Sokets_Task.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <winsock2.h>
#include <locale.h>
#include <ws2tcpip.h>

#pragma comment(lib,"ws2_32.lib") //Winsock Library

int saveReceivedFile(SOCKET s)
{
	char *fname = (char*)calloc(500, sizeof(char));
	FILE *fileForSaveData;
	puts("Куда сохранить пересылаемый файл? Введите путь.");
	gets(fname);
	while (fopen(fname, "w") == NULL)
	{
		puts("Путь введен неверно или туда невозможно сохранить файл, введите ещё раз.");
		free(fname);
		fname = (char*)calloc(500, sizeof(char));
		gets(fname);
	}
	fileForSaveData = fopen(fname, "w");
	free(fname);
	char *strFileLen = (char*)calloc(11, sizeof(char));
	int temp;
	if ((temp = recv(s, strFileLen, 11, 0)) != SOCKET_ERROR && temp != 0)
	{
		long fileLen = atol(strFileLen);
		char *fileData = (char*)malloc((fileLen+1)*sizeof(char));
		recv(s, fileData, fileLen, 0);
		fileData[fileLen] = '\0';
		fwrite(fileData, sizeof(char), fileLen+1, fileForSaveData);
		free(fileData);
	}
	puts("Файл успешно сохранен.");
	free(strFileLen);
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
			msgFromClient = (char*)calloc(500, sizeof(char));
			if ((recv_size = recv(new_socket, (char*)msgFromClient, 499, 0)) != SOCKET_ERROR)
			{
				if ((msgFromClient[0]) == 'f' && (msgFromClient[1]) == '1' && (msgFromClient[2]) == '3' && (msgFromClient[3]) == '3' && (msgFromClient[4]) == '7')
				{
					saveReceivedFile(new_socket);
				}
				else
				{
					puts("Message received:");
					//Add a NULL terminating character to make it a proper string before printing
					msgFromClient[recv_size] = '\0';
					puts((char*)msgFromClient);
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

