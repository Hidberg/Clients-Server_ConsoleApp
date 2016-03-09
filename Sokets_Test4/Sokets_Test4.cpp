// Sokets_Test4.cpp: определяет точку входа для консольного приложения.
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
	char *server_reply = (char*)calloc(500, sizeof(char));
	int temp;
	while ((temp = recv(s, server_reply, 499, 0)) != SOCKET_ERROR && temp != 0)
	{
		server_reply[temp] = '\0';
		fwrite(server_reply, sizeof(char), temp, fileForSaveData);
		if (temp < 499)
		{
			break; // Так мы узнаем что записываем последние данные передаваемого файла.
		}
		free(server_reply);
		server_reply = (char*)calloc(500, sizeof(char));
	}
	puts("Файл успешно сохранен.");
	free(server_reply);
	fclose(fileForSaveData);
	return 0;
}

int serverWork()
{
	WSADATA wsa;
	SOCKET s, new_socket;
	struct sockaddr_in server, client;
	int recv_size, addrlen, client_socket[3], max_clients = 3, activity, i, valread, sd, max_sd;
	char *server_reply;
	//set of socket descriptors
	fd_set readfds;

	//initialise all client_socket[] to 0 so not checked
	for (i = 0; i < max_clients; ++i)
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

	//Accept and incoming connection
	puts("Waiting for incoming connections...");

	addrlen = sizeof(struct sockaddr_in);

	while (TRUE)
	{
		//clear the socket set
		FD_ZERO(&readfds);

		//add master socket to set
		FD_SET(s, &readfds);
		max_sd = s;

		//add child sockets to set
		for (i = 0; i < max_clients; i++)
		{
			//socket descriptor
			sd = client_socket[i];

			//if valid socket descriptor then add to read list
			if (sd > 0)
				FD_SET(sd, &readfds);

			//highest file descriptor number, need it for the select function
			if (sd > max_sd)
				max_sd = sd;
		}

		//wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
		activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

		if ((activity < 0) /* && (errno != 0) */)
		{
			printf("select error");
		}

		//If something happened on the master socket , then its an incoming connection
		if (FD_ISSET(s, &readfds))
		{
			if ((new_socket = accept(s, (struct sockaddr *)&server, (socklen_t*)&addrlen)) < 0)
			{
				perror("accept");
				exit(EXIT_FAILURE);
			}

			//inform user of socket number - used in send and receive commands
			printf("New connection , socket fd is %d , ip is : %s , port : %d \n", new_socket, inet_ntoa(server.sin_addr), ntohs(server.sin_port));

			//add new socket to array of sockets
			for (i = 0; i < max_clients; i++)
			{
				//if position is empty
				if (client_socket[i] == 0)
				{
					client_socket[i] = new_socket;
					printf("Adding to list of sockets as %d\n", i);
					break;
				}
			}
		}

		//else its some IO operation on some other socket :)
		for (i = 0; i < max_clients; i++)
		{
			sd = client_socket[i];

			if (FD_ISSET(sd, &readfds))
			{
				server_reply = (char*)calloc(501, sizeof(char));
				
				//Check if it was for closing , and also read the incoming message
				while ((valread = recv(sd, server_reply, 499, 0)) != -1)
				{
					if (valread == 0)
					{
						//Somebody disconnected , get his details and print
						getpeername(sd, (struct sockaddr*)&server, (socklen_t*)&addrlen);
						printf("Host disconnected , ip %s , port %d \n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));

						//Close the socket and mark as 0 in list for reuse
						closesocket(sd);
						client_socket[i] = 0;
						free(server_reply);
						break;
					}
					else
					{
						if ((server_reply[0]) == 'f' && (server_reply[1]) == '1' && (server_reply[2]) == '3' && (server_reply[3]) == '3' && (server_reply[4]) == '7')
						{
							saveReceivedFile(sd);
							free(server_reply);
							break;
						}
						else
						{
							puts("Message received:");
							//Add a NULL terminating character to make it a proper string before printing
							server_reply[valread] = '\0';
							puts(server_reply);
							free(server_reply);
							if (valread < 500)
							{
								break;
							}
						}
					}
					
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
