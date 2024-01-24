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
#include <sqlite3.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

int encrypt_key = 3;
// Verificarea erorilor
// -------------------------------------------------------

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

void Recv(int socket, void *buffer, size_t length, int flags)
{
    int res = recv(socket, buffer, length, flags);
    if(res == -1)
    {
        perror("ERROR : recv() \n");
        exit(1);
    }
}

// -------------------------------------------------------

void Encrypt(char password[], int crypt_key)
{
    for(int i = 0; i < strlen(password); i++)
        password[i] = password[i] - crypt_key;
}

void Decrypt(char password[], int crypt_key)
{
    for(int i = 0; i < strlen(password); i++)
        password[i] = password[i] + crypt_key;
}

// functii pentru baza de date sqlite3
// -------------------------------------------------------

sqlite3 *db;

void OpenDatabase() {
    int rc = sqlite3_open("users.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
}

void CloseDatabase() {
    sqlite3_close(db);
}

int isAdminUserExisting() {
    char *checkAdminUserSQL = "SELECT COUNT(*) FROM users WHERE username = 'admin';";
    sqlite3_stmt *stmt;
    int count = 0;

    if (sqlite3_prepare_v2(db, checkAdminUserSQL, -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }

        sqlite3_finalize(stmt);
    } else {
        fprintf(stderr, "Error preparing SQL statement: %s\n", sqlite3_errmsg(db));
    }

    return count;
}

void CreateTable()
{
    char *sql = "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, email TEXT NOT NULL, password TEXT NOT NULL, username TEXT NOT NULL UNIQUE, friends TEXT, admin_key INTEGER, profile_type INTEGER, block INTEGER);";
    int rc = sqlite3_exec(db, sql, 0, 0, 0);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot create table: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    sql = "CREATE TABLE IF NOT EXISTS posts (id INTEGER PRIMARY KEY, user_id INTEGER, content TEXT, post_type INTEGER, created_at DATETIME DEFAULT CURRENT_TIMESTAMP);";
    rc = sqlite3_exec(db, sql, 0, 0, 0);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot create posts table: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    if (isAdminUserExisting() > 0) {
        printf("Admin user already exists.\n");
        return;
    }

    char password[] = "admin";
    Encrypt(password, encrypt_key);

    char insertAdminUserSQL[200];
    sprintf(insertAdminUserSQL, "INSERT INTO users (email, password, username, admin_key, profile_type) VALUES ('admin', '%s', 'admin', 1, 1);", password);

    rc = sqlite3_exec(db, insertAdminUserSQL, 0, 0, 0);

    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot insert admin user: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
}

void InsertUser(char *email, char *password, char *username) {
    char sql[200];
    snprintf(sql, sizeof(sql), "INSERT INTO users (email, password, username) VALUES ('%s', '%s', '%s');", email, password, username);
    int rc = sqlite3_exec(db, sql, 0, 0, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot insert user: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
}

int SearchUserInDatabase(char *email, char *password) {
    sqlite3_stmt *stmt;
    char sql[100];
    snprintf(sql, sizeof(sql), "SELECT * FROM users WHERE email = '%s' AND password = '%s';", email, password);

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    int result = 0;
    printf("email: %s, pass: %s \n", email, password);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        result = 1;
    }

    sqlite3_finalize(stmt);
    return result;
}

int EmailCheck(char *email)
{
    sqlite3_stmt *stmt;
    char sql[100];
    snprintf(sql, sizeof(sql), "SELECT * FROM users WHERE email = '%s';", email);

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    int result = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        result = 1;
    }

    sqlite3_finalize(stmt);
    return result;
}

int UsernameCheck(char *username)
{
    sqlite3_stmt *stmt;
    char sql[100];
    snprintf(sql, sizeof(sql), "SELECT * FROM users WHERE username = '%s';", username);

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    int result = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        result = 1;
    }

    sqlite3_finalize(stmt);
    return result;
}

void CreatePost(char *buffer, char *username, int post_type)
{
    char sql[1024];
    snprintf(sql, sizeof(sql), "INSERT INTO posts (user_id, content, post_type) VALUES ((SELECT id FROM users WHERE username = '%s'), '%s', %d);", username, buffer, post_type);
    int rc = sqlite3_exec(db, sql, 0, 0, 0);

    if (rc != SQLITE_OK)
    {
        perror("ERROR: post insertion \n");
        exit(1);
    }
}

void SetProfilePrivacy(char *buffer, char *email)
{
    char profile_type[2];
    sprintf(profile_type, "%d", strcmp(buffer, "setprofile_private") == 0 ? 1 : 0);

    char sql[1024];
    snprintf(sql, sizeof(sql), "UPDATE users SET profile_type = %s WHERE email = '%s';", profile_type, email);

    int rc = sqlite3_exec(db, sql, 0, 0, 0);
    if(rc != SQLITE_OK)
    {
        perror("ERROR : update profile type \n");
        exit(1);
    }
}

void AddFriendByUsername(char *user_username, char *friend_username, char *friend_type)
{
    char sql[1024];
    snprintf(sql, sizeof(sql), "UPDATE users SET friends = coalesce(friends, '') || '%s:%s\n' WHERE username = '%s';", friend_username, friend_type, user_username);
    int rc = sqlite3_exec(db, sql, 0, 0, 0);
    if (rc != SQLITE_OK)
    {
        perror("ERROR: adding friend \n");
        exit(1);
    }
}

void RemoveFriend(char *username, char *friend_username, char *friend_type)
{
    char sql[1024];
    snprintf(sql, sizeof(sql), "UPDATE users SET friends = replace(friends, '%s:%s\n', '') WHERE username = '%s';", friend_username, friend_type, username);


    int rc = sqlite3_exec(db, sql, 0, 0, 0);
    if (rc != SQLITE_OK)
    {
        perror("ERROR: failed to remove friend \n");
        exit(1);
    }
}

void GetFriends(char *username, char *friends_buffer)
{
    char sql[1024];
    snprintf(sql, sizeof(sql), "SELECT friends FROM users WHERE username = '%s';", username);

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        perror("ERROR: failed to get friends \n");
        exit(1);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *friends = (const char *)sqlite3_column_text(stmt, 0);
        printf("friends : %s", friends);
        strcpy(friends_buffer, friends);
    }

    sqlite3_finalize(stmt);

}

void UpdatePostType(int post_id, int post_type)
{
    char sql[1024];
    snprintf(sql, sizeof(sql), "UPDATE posts SET post_type = %d WHERE id = %d;", post_type, post_id);
    int rc = sqlite3_exec(db, sql, 0, 0, 0);

    if (rc != SQLITE_OK)
    {
        perror("ERROR: update post type \n");
        exit(1);
    }
}

// -------------------------------------------------------

void openChat()
{
    FILE *terminal = popen("konsole -e ./chat_server 5000 &", "r");
    if (terminal == NULL)
    {
        perror("popen");
        exit(EXIT_FAILURE);
    }
    pclose(terminal);
}

void *handleClient(void *arg)
{
    char err_msg[] = "Unknown command";
    char err_msg1[] = "This email is used, chose another one";
    char err_msg2[] = "You can't create a post as a guest";
    char err_msg3[] = "User with this username already exist, chose another username";
    char err_msg4[] = "User has a private profile";
    char err_msg5[] = "No user found";
    char err_msg6[] = "Profile was baned";
    char err_msg7[] = "Login failed";
    char err_msg8[] = "You are logged in. Logout first.";
    char err_msg9[] = "You are not autorized to use this feature";

    char msg1[] = "Post created successfully";
    char msg2[] = "Login successful";
    char msg3[] = "Logout succesful";
    char msg4[] = "Registration successful";
    char msg5[] = "Profile type updated successfully";


    int newSocket = *((int *)arg);
    char buffer[1024] = {0};
    char user_buffer[1024] = {0};
    char pass_buffer[1024] = {0};
    char email_buffer[1024] = {0};

    char email_memo[1024] = "";
    char pass_memo[1024] = "";
    char user_memo[1024] = "";
    char friend_type[1024] = "";
    char posts_buffer[5000] = "";
    bool key = 0;
    int id;

    while (1)
            {
            Recv(newSocket, buffer, 1024, 0);

            if (strcmp(buffer, "exit") == 0)
            {
                printf("Disconected \n");
                break;
            }
            else if (strcmp(buffer, "new_post") == 0)
            {
                if(key == 1)
                {
                    int post_size;
                    Recv(newSocket, &post_size, sizeof(int), 0);
                    Recv(newSocket, buffer, post_size, 0);

                    CreatePost(buffer, user_memo, 0);

                    printf("New post created \n");
                    send(newSocket, msg1, strlen(msg1), 0);
                }
                else
                {
                    printf("Error : new_post \n");
                    send(newSocket, err_msg2, strlen(err_msg2), 0);
                }
            }
            else if (strcmp(buffer, "posts") == 0)
            {
                Recv(newSocket, user_buffer, 1024, 0);
                if (UsernameCheck(user_buffer) == 1)
                {
                    char sql[1024];
                    snprintf(sql, sizeof(sql), "SELECT posts.post_type, posts.id, posts.content, posts.created_at, users.profile_type FROM posts INNER JOIN users ON posts.user_id = users.id WHERE users.username = '%s';", user_buffer);

                    sqlite3_stmt *stmt;
                    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
                    if (rc != SQLITE_OK)
                    {
                        perror("ERROR : failet command posts \n");
                        exit(1);
                    }
                    int profile_type = -1;

                    if(sqlite3_step(stmt) == SQLITE_ROW)
                        profile_type = sqlite3_column_int(stmt,4);

                    if(strcmp(user_memo, user_buffer) == 0)
                    {
                        sqlite3_reset(stmt);


                        while(sqlite3_step(stmt) == SQLITE_ROW)
                        {
                            int post_id = sqlite3_column_int(stmt, 1);
                            const char *content = (const char *)sqlite3_column_text(stmt, 2);
                            const char *created_at = (const char *)sqlite3_column_text(stmt, 3);
                            int post_type = sqlite3_column_int(stmt, 0);
                            printf("Post id : %d \n Post Type : %d \n Content : %s \n Created : %s \n", post_id, post_type, content, created_at);
                            if (post_type == 1)
                                strcat(posts_buffer, "Private: ");
                            else
                                strcat(posts_buffer, "Public: ");

                            char post_info[256];
                            snprintf(post_info, sizeof(post_info), "ID: %d, Content: %s, Created At: %s\n", post_id, content, created_at);
                            strcat(posts_buffer, post_info);
                        }

                        send(newSocket, posts_buffer, strlen(posts_buffer), 0);
                        printf("Get user posts \n");
                    }

                    if(profile_type != 1)
                    {
                        sqlite3_reset(stmt);
                        char posts_buffer[5000] = "";

                        while(sqlite3_step(stmt) == SQLITE_ROW)
                        {
                            int post_id = sqlite3_column_int(stmt, 1);
                            const char *content = (const char *)sqlite3_column_text(stmt, 2);
                            const char *created_at = (const char *)sqlite3_column_text(stmt, 3);
                            int post_type = sqlite3_column_int(stmt, 0);
                            printf("Post id : %d \n Post Type : %d \n Content : %s \n Created : %s \n", post_id, post_type, content, created_at);
                            if (post_type == 0)
                                strcat(posts_buffer, "Public: ");
                            else
                                continue;

                            char post_info[256];
                            snprintf(post_info, sizeof(post_info), "ID: %d, Content: %s, Created At: %s\n", post_id, content, created_at);
                            strcat(posts_buffer, post_info);
                        }

                        send(newSocket, posts_buffer, strlen(posts_buffer), 0);
                        printf("Get user posts \n");
                    }
                    else
                    {
                        printf("Error : posts \n");
                        send(newSocket, err_msg4, strlen(err_msg4), 0);
                    }
                    sqlite3_finalize(stmt);
                }
                else
                    send(newSocket, err_msg5, strlen(err_msg5), 0);
            }
            else if (strcmp(buffer, "login") == 0)
            {
                Recv(newSocket, email_buffer, 1024, 0);
                Recv(newSocket, pass_buffer, 1024, 0);

                Encrypt(pass_buffer, encrypt_key);

                char sql[1024];
                snprintf(sql, sizeof(sql), "SELECT username, block FROM users WHERE email = '%s';", email_buffer);

                sqlite3_stmt *stmt;
                int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
                if (rc != SQLITE_OK)
                {
                    perror("ERROR: failed to prepare statement\n");
                    exit(1);
                }

                int block_profile = -1;

                if (sqlite3_step(stmt) == SQLITE_ROW)
                    block_profile = sqlite3_column_int(stmt, 1);


                if (SearchUserInDatabase(email_buffer, pass_buffer) == 1 && strcmp(email_memo, "") == 0 && block_profile != 1)
                {
                    send(newSocket, msg2, strlen(msg2), 0);
                    key = 1;
                    sqlite3_reset(stmt);

                    if (sqlite3_step(stmt) == SQLITE_ROW)
                    {
                        const char *username = (const char *)sqlite3_column_text(stmt, 0);
                        strcpy(user_memo, username);
                    }

                    strcpy(email_memo, email_buffer);
                    strcpy(pass_memo, pass_buffer);
                }
                else if(block_profile == 1)
                    send(newSocket, err_msg6, strlen(err_msg6), 0);
                else
                    send(newSocket, err_msg7, strlen(err_msg7), 0);
            }
            else if (strcmp(buffer, "logout") == 0)
            {
                send(newSocket, msg3, strlen(msg3), 0);
                key = 0;
                strcpy(email_memo, "");
                strcpy(pass_memo, "");
                strcpy(user_memo, "");
            }
            else if (strcmp(buffer, "register") == 0)
            {
                if (strcmp(email_memo, "") == 0)
                {
                    Recv(newSocket, email_buffer, 1024, 0);
                    Recv(newSocket, pass_buffer, 1024, 0);
                    Recv(newSocket, user_buffer, 1024, 0);

                    Encrypt(pass_buffer, encrypt_key);

                    if (EmailCheck(email_buffer) == 1)
                        send(newSocket, err_msg1, strlen(err_msg1), 0);
                    else if (UsernameCheck(user_buffer) == 1)
                        send(newSocket, err_msg3, strlen(err_msg3), 0);
                    else
                    {
                        InsertUser(email_buffer, pass_buffer, user_buffer);
                        send(newSocket, msg4, strlen(msg4), 0);
                    }
                }
                else
                    send(newSocket, err_msg8, strlen(err_msg8), 0);
            }
            else if (strcmp(buffer, "setprofile_private") == 0 || strcmp(buffer, "setprofile_public") == 0)
            {
                if(key ==1)
                {
                    SetProfilePrivacy(buffer, email_memo);

                    printf("Profile updated \n");
                    send(newSocket, msg5, strlen(msg5), 0);
                }
                else
                {
                    printf("Error: Not authorized\n");
                    send(newSocket, err_msg9, strlen(err_msg9), 0);
                }
            }
            else if (strcmp(buffer, "setpost_private") == 0 || strcmp(buffer, "setpost_public") == 0)
            {
                if(key == 1)
                {
                    Recv(newSocket, &id, sizeof(id), 0);
                    char sql1[1024];
                    snprintf(sql1, sizeof(sql1), "SELECT user_id FROM posts WHERE id = '%d';", id);

                    char sql2[1024];
                    snprintf(sql2, sizeof(sql2), "SELECT id FROM users WHERE username = '%s';", user_memo);

                    sqlite3_stmt *stmt1;
                    int rc1 = sqlite3_prepare_v2(db, sql1, -1, &stmt1, 0);
                    if (rc1 != SQLITE_OK)
                    {
                        perror("ERROR: failed to prepare statement\n");
                        exit(1);
                    }

                    int user_id_posts = -1;

                    if (sqlite3_step(stmt1) == SQLITE_ROW)
                        user_id_posts = sqlite3_column_int(stmt1, 0);
                    //
                    sqlite3_stmt *stmt2;
                    int rc2= sqlite3_prepare_v2(db, sql2, -1, &stmt2, 0);
                    if (rc2 != SQLITE_OK)
                    {
                        perror("ERROR: failed to prepare statement\n");
                        exit(1);
                    }

                    int user_id_users = -1;
                    if (sqlite3_step(stmt2) == SQLITE_ROW)
                        user_id_users = sqlite3_column_int(stmt2, 0);

                    if( user_id_posts == user_id_users)
                    {
                        if(strcmp(buffer, "setpost_private") == 0)
                        {
                            UpdatePostType(id, 1);
                            send(newSocket, "Post type updated successfully", strlen("Post type updated successfully"), 0);
                        }
                        else
                        {
                            UpdatePostType(id, 0);
                            send(newSocket, "Post type updated successfully", strlen("Post type updated successfully"), 0);
                        }
                    }
                    else
                    {
                        printf("ERROR : Post type \n");
                        send(newSocket, "You are not owner of this post", strlen("You are not owner of this post"), 0);
                    }

                }
                else
                {
                    printf("Error: Not authorized\n");
                    send(newSocket, "You are not autorized to use this feature", strlen("You are not autorized to use this feature"), 0);
                }
            }
            else if (strcmp(buffer, "ban") == 0 || strcmp(buffer, "unban") == 0)
            {
                if(key == 1)
                {
                    Recv(newSocket, user_buffer, 1024, 0);

                    char sql[1024];
                    snprintf(sql, sizeof(sql), "SELECT admin_key FROM users WHERE username = '%s';", user_memo);

                    sqlite3_stmt *stmt;
                    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
                    if (rc != SQLITE_OK)
                    {
                        perror("ERROR: failed to prepare statement\n");
                        exit(1);
                    }

                    int admin_key = -1;

                    if (sqlite3_step(stmt) == SQLITE_ROW)
                        admin_key = sqlite3_column_int(stmt, 0);

                    printf("admin key : %d \n", admin_key);

                    if(admin_key == 1)
                    {
                        char block_profile[2];
                        sprintf(block_profile, "%d", strcmp(buffer, "ban") == 0 ? 1 : 0);
                        char sql1[1024];
                        snprintf(sql1, sizeof(sql1), "UPDATE users SET block = %s WHERE username = '%s';", block_profile, user_buffer);

                        int rc1 = sqlite3_exec(db, sql1, 0, 0, 0);
                        if(rc1 != SQLITE_OK)
                        {
                            perror("ERROR : update profile type \n");
                            exit(1);
                        }

                        if(strcmp(block_profile, "1") == 0)
                            printf("profile ban \n");
                        else
                            printf("profile unban \n");

                        printf("Profile updated \n");
                        send(newSocket, "profile block status changed", strlen("profile block status changed"), 0);
                    }
                    else
                        send(newSocket, "you don't have admin rights to use this command", strlen("you don't have admin rights to use this command"), 0);
                }
                else
                {
                    printf("Error: Not authorized\n");
                    send(newSocket, "You are not autorized to use this feature", strlen("You are not autorized to use this feature"), 0);
                }

            }
            else if (strcmp(buffer, "add_admin") == 0 || strcmp(buffer, "rm_admin") == 0)
            {
                if(key == 1)
                {
                    Recv(newSocket, user_buffer, 1024, 0);

                    char sql[1024];
                    snprintf(sql, sizeof(sql), "SELECT admin_key FROM users WHERE username = '%s';", user_memo);

                    sqlite3_stmt *stmt;
                    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
                    if (rc != SQLITE_OK)
                    {
                        perror("ERROR: failed to prepare statement\n");
                        exit(1);
                    }

                    int admin_key = -1;

                    if (sqlite3_step(stmt) == SQLITE_ROW)
                        admin_key = sqlite3_column_int(stmt, 0);

                    printf("admin key : %d \n", admin_key);

                    if(admin_key == 1)
                    {
                        char adm_key[2];
                        sprintf(adm_key, "%d", strcmp(buffer, "add_admin") == 0 ? 1 : 0);
                        char sql1[1024];
                        snprintf(sql1, sizeof(sql1), "UPDATE users SET admin_key = %s WHERE username = '%s';", adm_key, user_buffer);

                        int rc1 = sqlite3_exec(db, sql1, 0, 0, 0);
                        if(rc1 != SQLITE_OK)
                        {
                            perror("ERROR : update profile type \n");
                            exit(1);
                        }

                        if(strcmp(adm_key, "1") == 0)
                            printf("profile get admin rights \n");
                        else
                            printf("profile has been stripped of admin rights \n");

                        printf("Profile updated \n");
                        send(newSocket, "profile admin status changed", strlen("profile admin status changed"), 0);
                    }
                    else
                        send(newSocket, "you don't have admin rights to use this command", strlen("you don't have admin rights to use this command"), 0);
                }
                else
                {
                    printf("Error: Not authorized \n");
                    send(newSocket, "You are not autorized to use this feature", strlen("You are not autorized to use this feature"), 0);
                }
            }
            else if (strcmp(buffer, "add_friend") == 0)
            {
                if(key == 1)
                {
                    Recv(newSocket, user_buffer, 1024, 0);
                    Recv(newSocket, friend_type, 1024, 0);
                    if(UsernameCheck(user_buffer) == 1)
                    {
                        AddFriendByUsername(user_memo, user_buffer, friend_type);
                        printf("Friend added succesful \n");
                        char message[256];
                        snprintf(message, sizeof(message), "You added a new friend: %s as %s", user_buffer, friend_type);
                        send(newSocket, message, strlen(message),0);
                    }
                    else
                    {
                        printf("No user found \n");
                        send(newSocket, "Can't find user with this username", strlen("Can't find user with this username"), 0);
                    }
                }
                else
                {
                    printf("Error: Not authorized \n");
                    send(newSocket, "You are not autorized to use this feature", strlen("You are not autorized to use this feature"), 0);
                }
            }
            else if (strcmp(buffer, "rm_friend") == 0)
            {
                if(key == 1)
                {
                    Recv(newSocket, user_buffer, 1024, 0);
                    Recv(newSocket, friend_type, 1024, 0);

                    RemoveFriend(user_memo, user_buffer, friend_type);
                    printf("Done\n");
                    send(newSocket, "User removed from friends", strlen("User removed from friends"), 0);
                }
                else
                {
                    printf("Error: Not authorized \n");
                    send(newSocket, "You are not autorized to use this feature", strlen("You are not autorized to use this feature"), 0);
                }
            }
            else if (strcmp(buffer, "friends") == 0)
            {
                if(key == 1)
                {
                    char friends_buffer[5000] = "";
                    GetFriends(user_memo, friends_buffer);
                    send(newSocket, friends_buffer, 1024, 0);
                }
                else
                {
                    printf("Error: Not authorized \n");
                    send(newSocket, "You are not autorized to use this feature", strlen("You are not autorized to use this feature"), 0);
                }
            }
            else if (strcmp(buffer, "chat") == 0)
            {
                if(key == 1)
                {
                    printf("chat oppened \n");
                    send(newSocket, user_memo, strlen(user_memo), 0);
                }
                else
                {
                    printf("Error: Not authorized \n");
                    send(newSocket, "You are not autorized to use this feature", strlen("You are not autorized to use this feature"), 0);
                }
            }
            else
            {
                printf("Client: %s\n", buffer);
                send(newSocket, err_msg, strlen(err_msg), 0);
            }

            printf("email : %s | password : %s | username : %s \n", email_memo, pass_memo, user_memo);

            memset(buffer, 0, sizeof(buffer));
            memset(user_buffer, 0, sizeof(user_buffer));
            memset(pass_buffer, 0, sizeof(pass_buffer));
            memset(email_buffer, 0, sizeof(email_buffer));
            memset(friend_type, 0, sizeof(friend_type));
            memset(posts_buffer, 0, sizeof(posts_buffer));
        }

        close(newSocket);
        pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    OpenDatabase();
    CreateTable();


    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);

	int sockfd;
	struct sockaddr_in serverAddr;

	int newSocket;
	struct sockaddr_in newAddr;

	socklen_t addr_size;

	sockfd = Socket(AF_INET, SOCK_STREAM, 0);
	printf("Server created\n");

	memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	Bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	printf("Bind to port %d\n", port);

	Listen(sockfd, 10);
    openChat();
	while(1)
	{

		addr_size = sizeof(newAddr);
		newSocket = Accept(sockfd, (struct sockaddr*)&newAddr, &addr_size);

        printf("Connection accepted from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));

		pthread_t thread;
        int *newSocketPtr = malloc(sizeof(int));
        *newSocketPtr = newSocket;

        if (pthread_create(&thread, NULL, handleClient, (void *)newSocketPtr) != 0) {
            perror("Error creating thread\n");
            free(newSocketPtr);
            continue;
        }

        pthread_detach(thread);
	}
	CloseDatabase();
	return 0;
}
