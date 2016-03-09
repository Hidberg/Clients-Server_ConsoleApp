// Sokets_Test3.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <winsock2.h>
#include <locale.h>
#include <ws2tcpip.h>

#define PORT "58888"

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
	char *server_reply = (char*)calloc(501, sizeof(char));
	int temp;
	while ((temp = recv(s, server_reply, 500, 0)) != SOCKET_ERROR && temp != 0)
	{
		server_reply[temp + 1] = '\0';
		fwrite(server_reply, sizeof(char), temp, fileForSaveData);
		if (temp < 500)
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
	int c;
	char *server_reply;
	int recv_size;

	fd_set master;    // master file descriptor list
	fd_set read_fds;  // temp file descriptor list for select()
	int fdmax;        // maximum file descriptor number

	int newfd;        // newly accept()ed socket descriptor
	struct sockaddr_in remoteaddr; // client address
	socklen_t addrlen;
	int nbytes, rv;

	struct addrinfo hints, *ai, *p;

	FD_ZERO(&master);    // clear the master and temp sets
	FD_ZERO(&read_fds);

	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		return 1;
	}
	printf("Initialised.\n");

	//Prepare the sockaddr_in structure
	//server.sin_addr.s_addr = inet_addr("192.168.1.38");
	//server.sin_family = AF_INET;
	//server.sin_port = htons(58888);

	// get us a socket and bind it
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0)
	{
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}

	for (int i = 0; i < 3; ++i)
	{
		s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (s < 0)
		{
			continue;
		}

		if (bind(s, p->ai_addr, p->ai_addrlen) < 0)
		{
			closesocket(s);
			continue;
		}
		p = p->ai_next;
	}

	// if we got here, it means we didn't get bound
	if (p == NULL) {
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}

	freeaddrinfo(ai); // all done with this

	// listen
	if (listen(s, 3) == -1)
	{
		perror("listen");
		exit(3);
	}

	// add the listener to the master set
	FD_SET(s, &master);

	// keep track of the biggest file descriptor
	fdmax = s; // so far, it's this one

	c = sizeof(struct sockaddr_in);
	read_fds = master; // copy it

	while (TRUE)
	{
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
		{
			perror("select");
			exit(4);
		}

		// run through the existing connections looking for data to read
		for (int i = 0; i <= fdmax; ++i)
		{
			if (FD_ISSET(i, &read_fds)) // we got one!!
			{
				if (i == s) // handle new connections
				{
					addrlen = sizeof remoteaddr;
					newfd = accept(s, (struct sockaddr *)&remoteaddr, &addrlen);
					if (newfd == -1)
					{
						perror("accept");
					}
					else
					{
						FD_SET(newfd, &master); // add to master set
						if (newfd > fdmax) // keep track of the max
						{
							fdmax = newfd;
						}
						printf("selectserver: new connection from %s on "
							"socket %d\n",
							inet_ntop(remoteaddr.sin_family,
							&remoteaddr,
							(PSTR)INET6_ADDRSTRLEN, INET6_ADDRSTRLEN),
							newfd);
					}
				}
				else
				{
					//server_reply = (char*)calloc(501, sizeof(char));
					// handle data from a client
					if ((nbytes = recv(i, server_reply, sizeof(server_reply)-1, 0)) <= 0)
					{
						// got error or connection closed by client
						if (nbytes == 0)
						{
							// connection closed
							printf("selectserver: socket %d hung up\n", i);
						}
						else
						{
							perror("recv");
						}
						closesocket(i); // bye!
						FD_CLR(i, &master); // remove from master set
					}
					else
					{
						if ((server_reply[0]) == 'f' && (server_reply[1]) == '1' && (server_reply[2]) == '3' && (server_reply[3]) == '3' && (server_reply[4]) == '7')
						{
							saveReceivedFile(new_socket);
						}
						else
						{
							puts("Message received:");
							//Add a NULL terminating character to make it a proper string before printing
							server_reply[500] = '\0';
							puts(server_reply);
						}
					}
				} // END handle data from client
			} // END got new incoming connection
		} // END looping through file descriptors
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

