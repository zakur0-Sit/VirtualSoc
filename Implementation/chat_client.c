#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define LENGTH 2048

int flag = 0;
int sockfd = 0;
char name[32];

int Socket(int domain, int type, int protocol)
{
    int res = socket(domain, type, protocol);
    if (res == -1)
    {
        perror("ERROR : socket() \n");
        exit(1);
    }
    return res;
}

int Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    int res = connect(sockfd, addr, addrlen);
    if (res == -1)
    {
        perror("ERROR : connect() \n");
        exit(1);
    }
    return res;
}

int Recv(int socket, void *buffer, size_t length, int flags)
{
    int res = recv(socket, buffer, length, flags);
    if(res == -1)
    {
        perror("ERROR : recv() \n");
        exit(1);
    }
    return res;
}

void PthreadCreate(pthread_t *restrict thread, const pthread_attr_t *restrict attr, void *(*start_routine)(void *), void *restrict arg)
{
	int res = pthread_create(thread, attr, start_routine, arg);
	if(res == -1)
	{
		perror("ERROR : pthread_create() \n");
		exit(1);
	}
}

void StringOverwrite()
{
	printf("%s", "> ");
	fflush(stdout);
}

void TrimString (char* arr, int length)
{
	int i;
	for (i = 0; i < length; i++)
	{
		if (arr[i] == '\n')
		{
			arr[i] = '\0';
			break;
		}
	}
}

void send_msg_handler()
{
	char message[LENGTH] = {};
	char buffer[LENGTH + 32] = {};

	while(1)
	{
		StringOverwrite();
		fgets(message, LENGTH, stdin);
		TrimString(message, LENGTH);

		if (strcmp(message, "/exit") == 0)
			break;
		else
		{
			sprintf(buffer, "%s: %s\n", name, message);
			send(sockfd, buffer, strlen(buffer), 0);
		}

		memset(message, 0, sizeof(message));
		memset(buffer, 0, sizeof(buffer));
	}
	flag = 1;
}

void recv_msg_handler()
{
	char message[LENGTH] = {};
	while (1)
	{
		int receive = Recv(sockfd, message, LENGTH, 0);
		if (receive > 0)
		{
			printf("%s", message);
			StringOverwrite();
		}
		else{}
		memset(message, 0, sizeof(message));
	}
}

int main(int argc, char **argv)
{
	if(argc != 3)
	{
		printf("Usage: %s <port>\n", argv[0]);
		return 1;
	}

	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);
	strcpy(name, argv[2]);

	struct sockaddr_in server_addr;

	sockfd = Socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);

	Connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

	send(sockfd, name, 32, 0);

	pthread_t send_msg_thread;
	PthreadCreate(&send_msg_thread, NULL, (void *) send_msg_handler, NULL);

	pthread_t recv_msg_thread;
	PthreadCreate(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL);


	while (1)
	{
		if(flag)
			break;
	}

	close(sockfd);
	return 0;
}
