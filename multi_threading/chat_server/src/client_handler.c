#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "client_handler.h"
#include "db.h"
#include "online_users.h"
#include "file_transfer.h"
#include <errno.h>
#include <sys/time.h>

#define MAX_BUFFER 1024

void *client_handler(void *arg) {
    int newsockfd = *((int *)arg);
    free(arg);
    char buffer[MAX_BUFFER];
    ssize_t n;
    char user_id[128] = "";
    char username[32] = "";
    
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    // Set idle timeout of 180 seconds (3 minutes)
    struct timeval tv;
    tv.tv_sec = 500;
    tv.tv_usec = 0;
    if (setsockopt(newsockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
        perror("setsockopt failed");
    }




    if (getpeername(newsockfd, (struct sockaddr *)&addr, &addr_len) == -1) {
        perror("getpeername");
        close(newsockfd);
        pthread_exit(NULL);
    }
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    
    // Send welcome message and command instructions.
    send(newsockfd,
         "Welcome to Chat Server!\n"
         "Commands:\n"
         "  REGISTER <username> <password>\n"
         "  LOGIN <username> <password>\n"
         "  BROADCAST <message>\n"
         "  MSG <recipient_user_id> <message>\n"
         "  CHAT <recipient_user_id> <message>\n"
         "  GETCHAT <recipient_user_id>\n"
         "  LIST\n"
         "  ALLUSERS\n"
         "  FILE <recipient_user_id> <filename> <filesize>\n"
         "  LOGOUT\n", 350, 0);
    
    while (1) {
        memset(buffer, 0, MAX_BUFFER);
        n = read(newsockfd, buffer, MAX_BUFFER-1);
       if (n < 0) {
            // Check if the error was due to timeout.
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              send(newsockfd, "Idle timeout reached. Disconnecting...\n", 
              strlen("Idle timeout reached. Disconnecting...\n"), 0);

                printf("Client idle for more than 500 sec; disconnecting.\n");
            } else {
                perror("ERROR reading from socket");
            }
            break;
        } else if (n == 0) {
            printf("Client disconnected.\n");
            break;
        }
        buffer[n] = '\0';
        char *newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';
        
        char *command = strtok(buffer, " ");
        if (!command)
            continue;
           // ---------- REGISTER ----------
        if (strcasecmp(command, "REGISTER") == 0) {
            char *reg_username = strtok(NULL, " ");
            char *reg_password = strtok(NULL, " ");
            if (!reg_username || !reg_password) {
                send(newsockfd, "Usage: REGISTER <username> <password>\n", 40, 0);
                continue;
            }
            if (db_register_user(reg_username, reg_password, client_ip, user_id, sizeof(user_id))) {
                strncpy(username, reg_username, sizeof(username)-1);
                add_online_user(user_id, username, newsockfd);
                char msg[256];
                snprintf(msg, sizeof(msg), "Registration successful. Your User ID is: %s\n", user_id);
                send(newsockfd, msg, strlen(msg), 0);
            } else {
                send(newsockfd, "Registration failed. Username may already exist.\n", 50, 0);
            }
        }
        else if (strcasecmp(command, "LOGIN") == 0) {
            char *login_user_id = strtok(NULL, " ");
            char *login_password = strtok(NULL, " ");
            if (!login_user_id || !login_password) {
                send(newsockfd, "Usage: LOGIN <username> <password>\n", 35, 0);
                continue;
            }
            if (db_login_user(login_user_id, login_password, client_ip, username, sizeof(username))) {
                strncpy(user_id, login_user_id, sizeof(username)-1);
                add_online_user(user_id, username, newsockfd);
                char msg[256];
                snprintf(msg, sizeof(msg), "Login successful. Your User ID is: %s\n", user_id);
                send(newsockfd, msg, strlen(msg), 0);
                 // Deliver any offline messages.
                deliver_offline_messages(user_id, newsockfd);
            } else {
                send(newsockfd, "Login failed. Check your credentials.\n", 40, 0);
            }
        }
        else if (strcasecmp(command, "BROADCAST") == 0) {
            if (strlen(username) == 0) {
                send(newsockfd, "Please login first.\n", 21, 0);
                continue;
            }
            char *msg = strtok(NULL, "\0");
            if (!msg) {
                send(newsockfd, "Usage: BROADCAST <message>\n", 27, 0);
                continue;
            }
            broadcast_message(username, msg);
            send(newsockfd, "Broadcast sent.\n", 16, 0);
        }
        else if (strcasecmp(command, "MSG") == 0) {
            if (strlen(user_id) == 0) {
                send(newsockfd, "Please login first.\n", 21, 0);
                continue;
            }
            char *recipient_id = strtok(NULL, " ");
            char *msg = strtok(NULL, "\0");
            if (!recipient_id || !msg) {
                send(newsockfd, "Usage: MSG <recipient_user_id> <message>\n", 42, 0);
                continue;
            }
            char *history = db_get_chat_history(user_id, recipient_id);
            if (history) {
                send(newsockfd, "Previous chat history:\n", 25, 0);
                send(newsockfd, history, strlen(history), 0);
                free(history);
            } else {
                send(newsockfd, "No previous chat history found.\n", 33, 0);
            }
             int recipient_sock = get_online_sockfd(recipient_id);
            bool online = (recipient_sock != -1);
            if (!db_insert_chat(user_id, recipient_id, msg, online, online)) {
                send(newsockfd, "Failed to record message.\n", 26, 0);
                continue;
            }
           
            if (recipient_sock != -1) {
                char full_msg[MAX_BUFFER];
                snprintf(full_msg, sizeof(full_msg), "[Private from %s]: %s\n", username, msg);
                send(recipient_sock, full_msg, strlen(full_msg), 0);
            }
            send(newsockfd, "Message sent.\n", 14, 0);
        }
        else if (strcasecmp(command, "CHAT") == 0) {
            if (strlen(user_id) == 0) {
                send(newsockfd, "Please login first.\n", 21, 0);
                continue;
            }
            char *recipient_id = strtok(NULL, " ");
            char *msg = strtok(NULL, "\0");
            if (!recipient_id || !msg) {
                send(newsockfd, "Usage: CHAT <recipient_user_id> <message>\n", 43, 0);
                continue;
            }
            int recipient_sock = get_online_sockfd(recipient_id);
            bool online = (recipient_sock != -1);
            if (!db_insert_chat(user_id, recipient_id, msg, online, online)) {
                send(newsockfd, "Failed to record message.\n", 26, 0);
                continue;
            }
          
            if (recipient_sock != -1) {
                char full_msg[MAX_BUFFER];
                snprintf(full_msg, sizeof(full_msg), "[Private from %s]: %s\n", username, msg);
                send(recipient_sock, full_msg, strlen(full_msg), 0);
            }
            send(newsockfd, "Message sent.\n", 14, 0);
        }
        else if (strcasecmp(command, "GETCHAT") == 0) {
            char *other_id = strtok(NULL, " ");
            if (!other_id) {
                send(newsockfd, "Usage: GETCHAT <recipient_user_id>\n", 36, 0);
                continue;
            }
            char *history = db_get_chat_history(user_id, other_id);
            if (history) {
                send(newsockfd, history, strlen(history), 0);
                free(history);
            } else {
                send(newsockfd, "No chat history found.\n", 24, 0);
            }
        }
        else if (strcasecmp(command, "LIST") == 0) {
            // Implementation for listing online users.
            // (You can call functions from online_users.c to generate the list.)
                 if (strlen(user_id) == 0) {
                    send(newsockfd, "Please login first.\n", 21, 0);
                  continue;
              }
                 char list_buffer[MAX_BUFFER];
                 get_online_users_list(list_buffer, sizeof(list_buffer));
                 send(newsockfd, list_buffer, strlen(list_buffer), 0);
            
        }
        else if (strcasecmp(command, "ALLUSERS") == 0) {
            // Implementation for listing all registered users.
            // (This could use a query from db.c.)
            char *list_buffer = db_get_all_users();
            if (list_buffer) {
            send(newsockfd, list_buffer, strlen(list_buffer), 0);
             free(list_buffer);
              } else {
           send(newsockfd, "Error retrieving users.\n", 25, 0);
            }
        }
        else if (strcasecmp(command, "FILE") == 0) {
            // Implementation for file transfer.
             if (strlen(user_id) == 0) {
          send(newsockfd, "Please login first.\n", 21, 0);
        continue;
         }
     // Call the file transfer handler.
              handle_file_transfer(newsockfd, username);
        }
        else if (strcasecmp(command, "LOGOUT") == 0) {
            send(newsockfd, "Logging out. Goodbye!\n", 23, 0);
            break;
        }
        
        else {
            send(newsockfd, "Unknown command.\n", 17, 0);
        }
    }
    
    if (strlen(user_id) > 0)
        db_set_user_offline(user_id);
    remove_online_user(user_id);
    close(newsockfd);
    pthread_exit(NULL);
}

void create_client_thread(int *sockfd_ptr) {
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, client_handler, sockfd_ptr) != 0) {
         perror("Could not create client thread");
         free(sockfd_ptr);
    } else {
         pthread_detach(thread_id);
    }
}
