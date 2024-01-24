#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sqlite3.h>

#define MAX_CLIENTS 20
#define BUFFER_SZ 2048

int cli_count = 0;
static int uid = 10;

typedef struct
{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[32];
} client_t;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

sqlite3 *db;

void OpenDatabase() {
    int rc = sqlite3_open("messages.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
}

void CloseDatabase() {
    sqlite3_close(db);
}

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

void Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    int res = bind(sockfd, addr, addrlen);
    if (res == -1)
	{
        perror("ERROR : bind() \n");
        exit(1);
    }
}

void Listen(int sockfd, int backlog)
{
    int res = listen(sockfd, backlog);
    if (res == -1)
	{
        perror("ERROR : liste() \n");
        exit(1);
    }
}

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    int res = accept(sockfd, addr, addrlen);
    if (res == -1)
	{
        perror("ERROR : accept() \n");
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

void CreateTableMessages()
{
    char *sql = "CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY, sender_username TEXT, receiver_username TEXT, content TEXT);";
    int rc = sqlite3_exec(db, sql, 0, 0, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot create table: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
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

void AddToQueue(client_t *cl)
{
	pthread_mutex_lock(&clients_mutex);

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!clients[i])
		{
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

void RemoveFromQueue(int uid)
{
	pthread_mutex_lock(&clients_mutex);

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(clients[i])
		{
			if(clients[i]->uid == uid)
			{
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

void SendMessage(char *s, int uid)
{
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i<MAX_CLIENTS; ++i)
	{
		if(clients[i])
		{
			if(clients[i]->uid != uid)
			{
				if(write(clients[i]->sockfd, s, strlen(s)) < 0)
				{
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

void ChatHistory(int sockfd)
{
    char query[] = "SELECT content FROM messages WHERE receiver_username = 'all';";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, 0) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const char *content = sqlite3_column_text(stmt, 0);

            char message[BUFFER_SZ];
            snprintf(message, sizeof(message), "%s", content);

            if (write(sockfd, message, strlen(message)) < 0)
            {
                perror("ERROR: write to descriptor failed");
                break;
            }
        }

        sqlite3_finalize(stmt);
    }
    else
    {
        fprintf(stderr, "Error preparing SQL statement: %s\n", sqlite3_errmsg(db));
    }
}

void *handle_client(void *arg)
{
	char buff_out[BUFFER_SZ];
	char name[32];
	int leave_flag = 0;

	cli_count++;
	client_t *cli = (client_t *)arg;

	if(Recv(cli->sockfd, name, 32, 0))
	{
		strcpy(cli->name, name);
		sprintf(buff_out, "%s has joined\n", cli->name);
		printf("%s", buff_out);
		ChatHistory(cli->sockfd);
	} 

	memset(buff_out, 0, BUFFER_SZ);

	while(1)
	{
		if (leave_flag)
		{
			break;
		}

		int receive = Recv(cli->sockfd, buff_out, BUFFER_SZ, 0);

		if (receive > 0)
		{
			if (strlen(buff_out) > 0)
			{
				SendMessage(buff_out, cli->uid);

				char *sql = "INSERT INTO messages (sender_username, receiver_username, content) VALUES (?, ?, ?);";

				sqlite3_stmt *stmt;
				if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK)
				{
					sqlite3_bind_text(stmt, 1, cli->name, strlen(cli->name), 0);
					sqlite3_bind_text(stmt, 2, "all", 3, 0);
					sqlite3_bind_text(stmt, 3, buff_out, strlen(buff_out), 0);

					sqlite3_step(stmt);
					sqlite3_finalize(stmt);
				}
				else
				{
					fprintf(stderr, "Error preparing SQL statement: %s\n", sqlite3_errmsg(db));
				}

				TrimString(buff_out, strlen(buff_out));
				printf("%s \n", buff_out);
			}
		}

		else if (receive == 0 || strcmp(buff_out, "/exit") == 0)
		{
			sprintf(buff_out, "%s has left\n", cli->name);
			printf("%s", buff_out);
			SendMessage(buff_out, cli->uid);
			leave_flag = 1;
		}

		memset(buff_out, 0, BUFFER_SZ);
	}

	close(cli->sockfd);
    RemoveFromQueue(cli->uid);
    free(cli);
    cli_count--;
    pthread_detach(pthread_self());

	return NULL;
}

int main(int argc, char **argv)
{
	OpenDatabase();
	CreateTableMessages();
	if(argc != 2)
	{
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}

	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);
	int option = 1;
	int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    pthread_t tid;

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);

	if(setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0)
	{
		perror("ERROR: setsockopt failed");
		exit(1);
	}

    Bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    Listen(listenfd, 10);

	printf("Enter in the chat\n");

	while(1)
	{
		socklen_t clilen = sizeof(cli_addr);
		connfd = Accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		if((cli_count + 1) == MAX_CLIENTS)
		{
			printf("Max clients reached. Rejected: ");
			printf(":%d\n", cli_addr.sin_port);
			close(connfd);
			continue;
		}

		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;

		AddToQueue(cli);
		PthreadCreate(&tid, NULL, &handle_client, (void*)cli);
	}
	CloseDatabase();

	return 0;
}
