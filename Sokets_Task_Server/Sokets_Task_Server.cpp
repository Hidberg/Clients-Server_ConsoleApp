// Sokets_Task.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <winsock2.h>
#include <locale.h>
#include <ws2tcpip.h>

#pragma comment(lib,"ws2_32.lib") //Winsock Library

typedef enum { textMsg, fileMsg } msg_type;

void saveReceivedFile(SOCKET s, char *size_typeFromClient)
{
	char *fname = (char*)malloc(512 * sizeof(char));
	FILE *fileForSaveData;
	puts("Куда сохранить пересылаемый файл? Введите путь.");
	gets(fname);
	while (fopen(fname, "wb") == NULL)
	{
		puts("Путь введен неверно или туда невозможно сохранить файл, введите ещё раз.");
		gets(fname);
	}
	fileForSaveData = fopen(fname, "wb");
	free(fname);
	char *comingDataSize_str = (char*)malloc(4 * sizeof(char));
	for (int i = 1; i < 5; ++i)
	{
		comingDataSize_str[i - 1] = size_typeFromClient[i];
	}
	int comingDataSize = *((int*)comingDataSize_str);
	free(comingDataSize_str);
	int recv_size = 0, comingDataSize_copy = comingDataSize;
	char *fileData = (char*)malloc(comingDataSize*sizeof(char));
	while (comingDataSize_copy != 0 && (recv_size = recv(s, fileData + recv_size, comingDataSize_copy, 0)) != 0 && recv_size != SOCKET_ERROR)
	{
		comingDataSize_copy -= recv_size;
	}
	fwrite(fileData, sizeof(char), comingDataSize, fileForSaveData);
	printf("Файл успешно сохранен.	%i	%i\n", comingDataSize, comingDataSize_copy);
	free(fileData);
	fclose(fileForSaveData);
}

int serverWork()
{
	WSADATA wsa;
	SOCKET s, new_socket;
	struct sockaddr_in server;
	char *msgWithType_Size;
	fd_set readfds, copy_set;

	int max_clients = 10, client_socket[10];
	//initialise all client_socket[] to 0 so not checked
	for (int i = 0; i < max_clients; ++i)
	{
		client_socket[i] = 0;
	}
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
	listen(s, 3);

	puts("Waiting for incoming connections...");

	int addrlen;
	addrlen = sizeof(struct sockaddr_in);
	int max_sd, sd, activity;
	FD_ZERO(&readfds);
	FD_ZERO(&copy_set);
	FD_SET(s, &readfds);
	max_sd = s;

	while (TRUE)
	{
		copy_set = readfds;
		//wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
		activity = select(max_sd + 1, &copy_set, NULL, NULL, NULL);

		if (activity < 0)
		{
			printf("select error");
		}
		//If something happened on the master socket , then its an incoming connection
		if (FD_ISSET(s, &copy_set))
		{
			if ((new_socket = accept(s, (struct sockaddr *)&server, &addrlen)) < 0)
			{
				perror("accept");
				exit(EXIT_FAILURE);
			}

			//inform user of socket number - used in send and receive commands
			printf("New connection , socket fd is %d , ip is : %s , port : %d \n", new_socket, inet_ntoa(server.sin_addr), ntohs(server.sin_port));

			//add new socket to array of sockets
			int full = 1;
			for (int i = 0; i < max_clients; i++)
			{
				//if position is empty
				if (client_socket[i] == 0)
				{
					client_socket[i] = new_socket;
					if (new_socket > max_sd) max_sd = new_socket;
					FD_SET(new_socket, &readfds);
					printf("Adding to list of sockets as %d\n", i);
					full = 0;
					break;
				}
			}
			if (full)
			{
				closesocket(new_socket);
			}
		}
		else
		{
			//else its some IO operation on some other socket :)
			for (int i = 0; i < max_clients; ++i)
			{
				sd = client_socket[i];

				if (FD_ISSET(sd, &copy_set))
				{
					msgWithType_Size = (char*)malloc(5 * sizeof(char));
					int recv_size = 0;
					if ((recv_size = recv(sd, msgWithType_Size, 5, 0)) != SOCKET_ERROR && recv_size != 0)
					{
						if ((msgWithType_Size[0]) == fileMsg)
						{
							saveReceivedFile(sd, msgWithType_Size); // msgWithType_Size parametr in order to know sizeDataFile
						}
						else if ((msgWithType_Size[0]) == textMsg)
						{
							int comingDataSize = *((int*)(msgWithType_Size + 1));
							char *textMsg = (char*)malloc((comingDataSize + 1)*sizeof(char));
							recv_size = 0;
							puts("Message received:");
							while (comingDataSize != 0 && (recv_size = recv(sd, textMsg, comingDataSize, 0)) != 0 && recv_size != SOCKET_ERROR)
							{
								comingDataSize -= recv_size;
								textMsg[recv_size] = '\0';
								printf("%s", textMsg);
							}
							printf("\n");
							free(textMsg);
						}
					} 
					else
					{
						//Somebody disconnected , get his details and print
						getpeername(sd, (struct sockaddr*)&server, (socklen_t*)&addrlen);
						printf("Host disconnected , ip %s , port %d \n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));
						FD_CLR(sd, &readfds);
						//Close the socket and mark as 0 in list for reuse
						closesocket(sd);
						client_socket[i] = 0;
					}
					free(msgWithType_Size);
				}
			}
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

