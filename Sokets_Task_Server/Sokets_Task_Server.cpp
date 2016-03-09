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
	//unsigned char *server_reply = (unsigned char*)calloc(500, sizeof(unsigned char));
	unsigned char *fileLen = (unsigned char*)calloc(4, sizeof(unsigned char));
	int temp;
	if ((temp = recv(s, (char*)fileLen, 5, 0)) != SOCKET_ERROR && temp != 0)
	{
		char *fileData = (char*)malloc(atol((char*)fileLen)*sizeof(char));
		recv(s, fileData, atol((char*)fileLen), 0);
		fwrite(fileData, sizeof(unsigned char), atol((char*)fileLen), fileForSaveData);
		free(fileData);
		/*if (temp < 499)
		{
			break; // Так мы узнаем что записываем последние данные передаваемого файла.
		}*/
		//free(server_reply);
		//server_reply = (unsigned char*)calloc(500, sizeof(unsigned char));
	}
	puts("Файл успешно сохранен.");
	free(fileLen);
	fclose(fileForSaveData);
	return 0;
}

int serverWork()
{
	WSADATA wsa;
	SOCKET s, new_socket;
	struct sockaddr_in server, client;
	int c;
	unsigned char *server_reply;
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
			server_reply = (unsigned char*)calloc(500, sizeof(unsigned char));
			if ((recv_size = recv(new_socket, (char*)server_reply, 499, 0)) != SOCKET_ERROR)
			{
				if ((server_reply[0]) == 'f' && (server_reply[1]) == '1' && (server_reply[2]) == '3' && (server_reply[3]) == '3' && (server_reply[4]) == '7')
				{
					saveReceivedFile(new_socket);
				}
				else
				{
					puts("Message received:");
					//Add a NULL terminating character to make it a proper string before printing
					server_reply[recv_size] = '\0';
					puts((char*)server_reply);
				}
			}
			free(server_reply);
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

