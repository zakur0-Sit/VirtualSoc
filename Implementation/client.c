#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>

// Verificarea erorilor
// ---------------------------------------------------------------------

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

void Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    int res = connect(sockfd, addr, addrlen);
    if (res == -1)
    {
        perror("ERROR : connect() \n");
        exit(1);
    }
}

void Recv(int socket, void *buffer, size_t length, int flags)
{
    int res = recv(socket, buffer, length, flags);
    if(res == -1)
    {
        perror("ERROR : recv() \n");
        exit(1);
    }
}


// ---------------------------------------------------------------------

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
	int client_sock;
	struct sockaddr_in serverAddr;
	char buffer[1024];
    char user_buffer[1024] = {0};
    char pass_buffer[1024] = {0};
    char email_buffer[1024] = {0};
    char posts_buffer[5000] = {0};
	char post[2000];
    char user_memo[1024];
    char friend_type[1024] = "";

    bool key = 0;
    int id;

	client_sock = Socket(AF_INET, SOCK_STREAM, 0);
	printf("client sock create.\n");

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	Connect(client_sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	printf("connected to server \n");

	while (1)
    {
        printf("client: \t");
        scanf("%s", &buffer[0]);
        send(client_sock, buffer, strlen(buffer), 0);

        if (strcmp(buffer, "exit") == 0)
        {
            close(client_sock);
            printf("disconnected from server \n");
            exit(1);
        }
        else if (strcmp(buffer, "new_post") == 0)
        {
            if(key == 1)
            {
                printf("Enter your post: ");
                scanf(" %[^\n]", post);

                int post_size = strlen(post);
                send(client_sock, &post_size, sizeof(int), 0);

                send(client_sock, post, post_size, 0);

                Recv(client_sock, buffer, 1024, 0);
                printf("server: \t%s\n", buffer);
            }
            else
            {
                Recv(client_sock, buffer, 1024, 0);
                printf("server: \t%s\n", buffer);
            }
        }
        else if (strcmp(buffer, "posts") == 0)
        {
            printf("Whose posts do you want to get ? \n");
            printf("User : ");
            scanf("%s", user_buffer);

            send(client_sock, user_buffer, strlen(user_buffer), 0);

            Recv(client_sock, posts_buffer, sizeof(posts_buffer), 0);
            printf("All posts from user %s: \n%s\n", user_buffer, posts_buffer);
        }
        else if (strcmp(buffer, "login") == 0)
        {
            printf("Enter your email : ");
            scanf("%s", email_buffer);

            send(client_sock, email_buffer, strlen(email_buffer), 0);

            printf("Enter your password : ");
            scanf("%s", pass_buffer);

            send(client_sock, pass_buffer, strlen(pass_buffer), 0);

            Recv(client_sock, buffer, 1024, 0);
            printf("server: \t%s\n", buffer);
            if(strcmp(buffer, "Login successful") == 0)
            {
                key = 1;

            }
        }
        else if (strcmp(buffer, "logout") == 0)
        {
            Recv(client_sock, buffer, 1024, 0);
            printf("server: \t%s\n", buffer);
            key = 0;
        }
        else if (strcmp(buffer, "register") == 0)
        {
            printf("Chose an email : ");
            scanf("%s", email_buffer);

            send(client_sock, email_buffer, strlen(email_buffer), 0);

            printf("Chose a password : ");
            scanf("%s", pass_buffer);

            send(client_sock, pass_buffer, strlen(pass_buffer), 0);

            printf("Enter your username : ");
            scanf("%s", user_buffer);

            send(client_sock, user_buffer, strlen(user_buffer), 0);

            Recv(client_sock, buffer, 1024, 0);
            printf("server: \t%s\n", buffer);
        }
        else if (strcmp(buffer, "setprofile_private") == 0 || strcmp(buffer, "setprofile_public") == 0)
        {
            Recv(client_sock, buffer, 1024, 0);
            printf("server: \t%s\n", buffer);
        }
        else if (strcmp(buffer, "setpost_private") == 0 || strcmp(buffer, "setpost_public") == 0)
        {
            if(key == 1)
            {
                printf("Post ID : ");
                scanf("%d", &id);
                send(client_sock, &id, sizeof(id), 0);
                Recv(client_sock, buffer, 1024, 0);
                printf("server: \t%s\n", buffer);
            }
            else
            {
                Recv(client_sock, buffer, 1024, 0);
                printf("server: \t%s\n", buffer);
            }
        }
        else if (strcmp(buffer, "ban") == 0 || strcmp(buffer, "unban") == 0)
        {
            if(key == 1)
            {
                printf("Which user should be baned or unbaned? \n");
                printf("User : ");
                scanf("%s", user_buffer);

                send(client_sock, user_buffer, strlen(user_buffer), 0);
                Recv(client_sock, buffer, 1024, 0);
                printf("server: \t%s\n", buffer);
            }
            else
            {
                Recv(client_sock, buffer, 1024, 0);
                printf("server: \t%s\n", buffer);
            }
        }
        else if (strcmp(buffer, "add_admin") == 0 || strcmp(buffer, "rm_admin") == 0)
        {
            if(key == 1)
            {
                printf("Which user should receive or be deprived of admin rights \n");
                printf("User : ");
                scanf("%s", user_buffer);

                send(client_sock, user_buffer, strlen(user_buffer), 0);
                Recv(client_sock, buffer, 1024, 0);
                printf("server: \t%s\n", buffer);
            }
            else
            {
                Recv(client_sock, buffer, 1024, 0);
                printf("server: \t%s\n", buffer);
            }
        }
        else if (strcmp(buffer, "add_friend") == 0)
        {
            if(key == 1)
            {
                printf("Which user do you want to add to friends \n");
                printf("User : ");
                scanf("%s", user_buffer);
                printf("What type of friend is the user \n");
                printf("Type : ");
                scanf("%s", friend_type);

                send(client_sock, user_buffer, 1024, 0);
                send(client_sock, friend_type, 1024, 0);
                Recv(client_sock, buffer, 1024, 0);
                printf("server: \t%s\n", buffer);
            }
            else
            {
                Recv(client_sock, buffer, 1024, 0);
                printf("server: \t%s\n", buffer);
            }
        }
        else if (strcmp(buffer, "rm_friend") == 0)
        {
            if(key == 1)
            {
                printf("Which user do you want to remove from friends \n");
                printf("User : ");
                scanf("%s", user_buffer);
                printf("What type of friend is the user \n");
                printf("Type : ");
                scanf("%s", friend_type);

                send(client_sock, user_buffer, 1024, 0);
                send(client_sock, friend_type, 1024, 0);
                Recv(client_sock, buffer, 1024, 0);
                printf("server: \t%s\n", buffer);
            }
            else
            {
                Recv(client_sock, buffer, 1024, 0);
                printf("server: \t%s\n", buffer);
            }
        }
        else if (strcmp(buffer, "friends") == 0)
        {
            if(key == 1)
            {
                printf("All user's friends : \n");
                Recv(client_sock, buffer, 1024, 0);
                printf("\n%s\n", buffer);
            }
            else
            {
                Recv(client_sock, buffer, 1024, 0);
                printf("server: \t%s\n", buffer);
            }
        }
        else if (strcmp(buffer, "chat") == 0)
        {
            if(key == 1)
            {
                printf("Chat opened \n");
                Recv(client_sock, user_memo, 1024, 0);
                char command[1024];
                char terminal_path[] = "konsole -e ./chat_client 5000 ";

                sprintf(command, "%s %s", terminal_path, user_memo);

                FILE *terminal = popen(command, "r");
                if (terminal == NULL)
                {
                    perror("popen");
                    exit(EXIT_FAILURE);
                }
                pclose(terminal);
            }
            else
            {
                Recv(client_sock, buffer, 1024, 0);
                printf("server: \t%s\n", buffer);
            }
        }
        else
        {
            Recv(client_sock, buffer, 1024, 0);
            printf("server: \t%s\n", buffer);
        }

        memset(buffer, 0, sizeof(buffer));
        memset(user_buffer, 0, sizeof(user_buffer));
        memset(pass_buffer, 0, sizeof(pass_buffer));
        memset(email_buffer, 0, sizeof(email_buffer));
        memset(posts_buffer, 0, sizeof(posts_buffer));
        memset(post, 0, sizeof(post));
        memset(friend_type, 0, sizeof(friend_type));

    }

	return 0;
}
