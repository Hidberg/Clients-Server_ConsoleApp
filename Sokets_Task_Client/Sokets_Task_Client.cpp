// Sokets_Task_Client.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include "winsock2.h"
#include "io.h"
#include "locale.h"

#pragma comment(lib,"ws2_32.lib") //Winsock Library

const int headerSize = 5;
typedef enum {textMsg, fileMsg} msg_type;

int loadInfoFromConfig(struct sockaddr_in *server)
{
	int status = 0;
	FILE *config = NULL;
	char *strFromConfig = NULL;
	char *fname = "config.txt"; 
	if (_access(fname, 0) != 0)
	{
		fname = "D:/Projects on C/Tasks_From_Muxan/Debug/config.txt";
		if (_access(fname, 0) != 0)
		{
			printf("Конфиг должен быть в одной папке с клиентом.\n");
			status = -1;
		}
	}
	if (status == 0)
	{
		config = fopen(fname, "r");
		strFromConfig = (char*)calloc(17, sizeof(char));
		if (fgets(strFromConfig, 17, config) != NULL)
		{
			if (inet_addr(strFromConfig) != -1)
			{
				(*server).sin_addr.s_addr = inet_addr(strFromConfig);
			}
			else
			{
				printf("Проверьте правильность написания ip - адреса в конфиге.\n");
				status = -2;
			}
		}
		else
		{
			printf("Скорее всего файл с конфигом пустой.\n");
			status = -2;
		}
	}
	if (status == 0)
	{
		free(strFromConfig);
		strFromConfig = (char*)calloc(17, sizeof(char));
		if (fgets(strFromConfig, 6, config) != NULL)
		{
			int port = atoi(strFromConfig);
			(*server).sin_port = htons(port);
			(*server).sin_family = AF_INET;
		}
		else
		{
			printf("Введите номера порта в config для подключения.\n");
			status = -2;
		}
	}
	if (status != 1)
	{
		free(strFromConfig);
		fclose(config);
	}
	return status;
}

int checkFileSend(char *messageToSend)
{
	if ((messageToSend[0]) == 'f' && (messageToSend[1]) == ' ')
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int sendFile(char *messageWithPath, SOCKET serverSock)
{
	char *pathFile = (messageWithPath + 2);
	FILE *fileForSend;
	if (fopen(pathFile, "rb") != NULL)
	{
		fileForSend = fopen(pathFile, "rb");
	}
	else
	{
		printf("Путь для файла указан неверно или файл не существует.\n");
		return -1;
	}

	long sizeofFile;
	// Определяем размер файла.
	fseek(fileForSend, 0, SEEK_END);
	sizeofFile = ftell(fileForSend);
	fseek(fileForSend, 0, SEEK_SET);
	char *p = (char*)&sizeofFile;

	char *dataForSend = (char*)malloc((headerSize + sizeofFile) * sizeof(char));
	dataForSend[0] = fileMsg;
	for (int i = 0; i < 4; ++i)
	{
		dataForSend[1 + i] = *(p+i);
	}
	int temp = fread((dataForSend + headerSize), 1, sizeofFile, fileForSend);
	if (send(serverSock, dataForSend, headerSize + sizeofFile, 0) < 0)
	{
		puts("Передача файла прервалась, возможно прервался connection.");
		free(dataForSend);
		fclose(fileForSend);
		return -1;
	}
	free(dataForSend);
	puts("Файл отправлен.");
	fclose(fileForSend);
	return 0;
}

int clientWork()
{
	WSADATA wsa;
	SOCKET s;
	struct sockaddr_in server;

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
		WSACleanup();
	}
	printf("Socket created.\n");

	// Initialise a server parametr 
	server.sin_addr.s_addr = inet_addr("127.0.0.1"); 
	server.sin_family = AF_INET;
	server.sin_port = htons(1111);
	// There is not meaningful parametrs

	while (loadInfoFromConfig(&server) != 0)
	{
		printf("Исправьте ошибку и введите любой символ для продолжения.\n");
		getchar();
	}

	//Connect to remote server
	if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		puts("connect error");
		closesocket(s);
		WSACleanup();
		return 1;
	}
	puts("Connected");
	char *message = (char*)calloc(499, sizeof(char));

	//Send some data
	while (gets(message+headerSize))
	{
		char *temp = message+headerSize;
		if (checkFileSend(temp) == 0)
		{
			if (sendFile(temp, s) == 0)
			{
				; // Нужно сделать проверку соединения иначе при потере connection программа завершится.
			}
		}
		else
		{
			int sizeofMessage = strlen(temp);
			message[0] = textMsg;
			char *p = (char*)&(sizeofMessage);
			for (int i = 0; i < 4; ++i)
			{
				message[1 + i] = *(p + i);
			}
			if (send(s, message, sizeofMessage + headerSize, 0) < 0)
			{
				puts("Send failed");
				free(message);
				return 1;
			}
			puts("Data Send");
		}
		// Обнуление строки message.
		for (int i = 0; i < 499; ++i)
		{
			message[i] = 0;
		}
	}
	closesocket(s);
	WSACleanup();
	return 0;
}

int main()
{
	setlocale(LC_ALL, "rus");
	clientWork();
	return 0;
}

