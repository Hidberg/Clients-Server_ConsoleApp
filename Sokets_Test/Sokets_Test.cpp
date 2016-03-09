// Sokets_Test.cpp: определяет точку входа для консольного приложения.

// SERVER v.Test

#include "stdafx.h"
#include <winsock2.h>
#include <locale.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

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
		fname = (char*)calloc(500, sizeof(char));
		gets(fname);
	}
	fileForSaveData = fopen(fname, "w");
	char *server_reply = (char*)calloc(501, sizeof(char));
	int temp;
	while ((temp = recv(s, server_reply, 500, 0)) != SOCKET_ERROR && temp != 0)
	{
		server_reply[temp + 1] = '\0';
		fwrite(server_reply, sizeof(char), strlen(server_reply), fileForSaveData);
		if (temp < 500)
		{
			break; // Так мы узнаем что записываем последние данные передаваемого файла.
		}
		server_reply = (char*)calloc(500, sizeof(char));
	}
	puts("Файл успешно сохранен.");
	fclose(fileForSaveData);
	return 0;
}

#define PORT "58888"   // port we're listening on

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int serverWork()
{
	fd_set master;    // master file descriptor list
	fd_set read_fds;  // temp file descriptor list for select()
	int fdmax;        // maximum file descriptor number

	int listener;     // listening socket descriptor
	int newfd;        // newly accept()ed socket descriptor
	struct sockaddr_storage remoteaddr; // client address
	socklen_t addrlen;
	WSADATA wsa;

	char *buf = (char*)calloc(501, sizeof(char));    // buffer for client data
	int nbytes;

	char remoteIP[INET6_ADDRSTRLEN];

	int yes = 1;        // for setsockopt() SO_REUSEADDR, below
	int i, j, rv;

	struct addrinfo hints, *ai, *p;

	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		return 1;
	}
	printf("Initialised.\n");
	
	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) 
	{
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}

	for (p = ai; p != NULL; p = p->ai_next) 
	{
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) 
		{
			continue;
		}

		// lose the pesky "address already in use" error message
		//setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) 
		{
			shutdown(listener, 2);
			continue;
		}

		break;
	}

	// if we got here, it means we didn't get bound
	if (p == NULL) 
	{
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}

	freeaddrinfo(ai); // all done with this

	// listen
	if (listen(listener, 10) == -1) 
	{
		perror("listen");
		exit(3);
	}

	// add the listener to the master set
	FD_SET(listener, &master);

	// keep track of the biggest file descriptor
	fdmax = listener; // so far, it's this one

	// main loop
	for (;;) 
	{
		read_fds = master; // copy it
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) 
		{
			perror("select");
			exit(4);
		}

		// run through the existing connections looking for data to read
		for (i = 0; i <= fdmax; i++) 
		{
			if (FD_ISSET(i, &read_fds)) 
			{ // we got one!!
				if (i == listener) 
				{
					// handle new connections
					addrlen = sizeof remoteaddr;
					newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
					if (newfd == -1) 
					{
						perror("accept");
					}
					else 
					{
						FD_SET(newfd, &master); // add to master set
						if (newfd > fdmax) 
						{    // keep track of the max
							fdmax = newfd;
						}
						printf("selectserver: new connection from %s on socket %d\n",
							inet_ntop(remoteaddr.ss_family,
							get_in_addr((struct sockaddr*)&remoteaddr),
							remoteIP, INET6_ADDRSTRLEN),
							newfd);
					}
				}
				else 
				{
					// handle data from a client
					if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0) 
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
						shutdown(i, 2); // bye!
						FD_CLR(i, &master); // remove from master set
					}
					else 
					{
						// we got some data from a client
						if ((buf[0]) == 'f' && (buf[1]) == '1' && (buf[2]) == '3' && (buf[3]) == '3' && (buf[4]) == '7')
						{
							saveReceivedFile(i);
						}
						else
						{
							puts("Message received:");
							//Add a NULL terminating character to make it a proper string before printing
							buf[500] = '\0';
							puts(buf);
							free(buf);
							buf = (char*)calloc(501, sizeof(char));
						}
					}
				} // END handle data from client
			} // END got new incoming connection
		} // END looping through file descriptors
	} // END for(;;)--and you thought it would never end!

	WSACleanup();
	return 0;
}

int main()
{
	setlocale(LC_ALL, "rus");
	serverWork();
	return 0;
}

