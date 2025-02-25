#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "client.h"
#include "receive_handler.h"
#include "ui.h"
#include "file_transfer.h"



int sockfd;  // Global socket descriptor

int main(int argc, char *argv[]) {
   printf("\033[2J");  // Clear screen
     const char chat_logo[] =


"██████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████\n\n"


                       "\t\t                                  ██████╗██╗  ██╗ █████╗ ████████╗     █████╗ ██████╗ ██████╗ \n"
                       "\t\t                                 ██╔════╝██║  ██║██╔══██╗╚══██╔══╝    ██╔══██╗██╔══██╗██╔══██╗\n"
                       "\t\t                                 ██║     ███████║███████║   ██║       ███████║██████╔╝██████╔╝\n"
                       "\t\t                                 ██║     ██╔══██║██╔══██║   ██║       ██╔══██║██╔═══╝ ██╔═══╝ \n"
                       "\t\t                                 ╚██████╗██║  ██║██║  ██║   ██║       ██║  ██║██║     ██║     \n"
                       "\t\t                                 ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝       ╚═╝  ╚═╝╚═╝     ╚═╝     \n"


"██████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████\n";

const char thank[]=

"                                ███╗````````████████╗██╗``██╗███████╗███╗```██╗██╗``██╗██╗```██╗██╗```██╗````````███╗ \n"
"                                ██╔╝````````╚══██╔══╝██║``██║██╔════╝████╗``██║██║`██╔╝██║```██║██║```██║````````╚██║ \n"
"                                ██║````````````██║```███████║█████╗``██╔██╗`██║█████╔╝`██║```██║██║```██║`````````██║ \n"
"                                ██║````````````██║```██╔══██║██╔══╝``██║╚██╗██║██╔═██╗`██║```██║██║```██║`````````██║ \n"
"                                ███╗```````````██║```██║``██║███████╗██║`╚████║██║``██╗╚██████╔╝╚██████╔╝````````███║ \n"
"                                ╚══╝```````````╚═╝```╚═╝``╚═╝╚══════╝╚═╝``╚═══╝╚═╝``╚═╝`╚═════╝``╚═════╝`````````╚══╝ \n";


printf("%s",chat_logo);

    if(argc < 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        exit(1);
    }
    char *server_ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in serv_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
         perror("ERROR opening socket");
         exit(1);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
         perror("Invalid address");
         exit(1);
    }
    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
         perror("Connection failed");
         exit(1);
    }

    // Start the receiver thread.
    pthread_t recv_thread;
    if(pthread_create(&recv_thread, NULL, receive_handler, NULL) != 0) {
         perror("Could not create receive thread");
         exit(1);
    }

    char username[32];
    // Authenticate (Register or Login)
    authenticate(sockfd, username, sizeof(username));

    char option[10];
    char buffer[MAX_BUFFER];
    while (1) {
        print_menu();
    
        if (!fgets(option, sizeof(option), stdin))
            continue;
        int menu_choice = atoi(option);
        if(menu_choice == 1) {  // Broadcast Message
            printf("   \033[4mEnter message to broadcast:\033[0m ");
            if (!fgets(buffer, sizeof(buffer), stdin))
                continue;
            buffer[strcspn(buffer, "\n")] = '\0';
            char command[MAX_BUFFER];
            snprintf(command, sizeof(command), "BROADCAST %s\n", buffer);
            // printf("Debugging :: command %s\n",command);
            write(sockfd, command, strlen(command));
            sleep(1);
        }
        else if(menu_choice == 2) {  // Private Chat Session (Continuous)
            char recipient[128];
            printf("   \033[4mEnter recipient user ID to chat with:\033[0m  ");
            if (!fgets(recipient, sizeof(recipient), stdin))
                continue;
            recipient[strcspn(recipient, "\n")] = '\0';
            // Retrieve previous chat history.
            snprintf(buffer, sizeof(buffer), "GETCHAT %s\n", recipient);
            printf("\n\n\033[1;34m~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Previous chat history ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \033[0m\n");  // Blue text
            
            write(sockfd, buffer, strlen(buffer));
            sleep(1);
            // Enter chat session.
            printf("\n\n                         \033[1;32m!!!!!!!!!!!!!!!!!!  ~~~~~~~~~     Chat session with %s ~~~~~~~~~  !!!!!!!!!!!!!!!!!!\033[0m\n\n", recipient);
            printf("                         Type your messages below. \033[1;31mType \033[1m\"/exit\"\033[0m on a new line to leave the chat.\033[0m\n");
            while (1) {
                char chatMsg[MAX_BUFFER];
                printf("[%s] >> ", username);
                if (!fgets(chatMsg, sizeof(chatMsg), stdin))
                    continue;
                chatMsg[strcspn(chatMsg, "\n")] = '\0';
                if (strcmp(chatMsg, "/exit") == 0)
                    break;
                char chatCommand[MAX_BUFFER];
                snprintf(chatCommand, sizeof(chatCommand), "CHAT %s %s\n", recipient, chatMsg);
                write(sockfd, chatCommand, strlen(chatCommand));
            }
            printf("\033[1;31mExited chat session with %s.\033[0m\n", recipient);
        }
        else if(menu_choice == 3) {  // Retrieve Chat History (Standalone)
            char recipient[128];
            printf("\033[4mEnter recipient user ID to retrieve chat history:\033[0m   ");
            if (!fgets(recipient, sizeof(recipient), stdin))
                continue;
            recipient[strcspn(recipient, "\n")] = '\0';
            snprintf(buffer, sizeof(buffer), "GETCHAT %s\n", recipient);
              printf("\n\n\033[1;34m~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Previous chat history ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \033[0m\n");  // Blue text
            write(sockfd, buffer, strlen(buffer));
            sleep(1);
        }
        else if(menu_choice == 4) {  // List Online Users
          printf("\033[1;32m ***********************     Online Users  ***********************     \033[0m\n"); // Green text
            snprintf(buffer, sizeof(buffer), "LIST\n");
            write(sockfd, buffer, strlen(buffer));
            sleep(1);
        }
        else if(menu_choice == 5) {  // List All Registered Users
            printf("\033[1;34m` ***********************   All Registered Users  ***********************    \033[0m\n");  // Blue text
            snprintf(buffer, sizeof(buffer), "ALLUSERS\n");
            write(sockfd, buffer, strlen(buffer));
            sleep(1);
        }
        else if(menu_choice == 6) {  // File Transfer
            send_file(sockfd);
        }
        else if(menu_choice == 7) {  // Logout
            snprintf(buffer, sizeof(buffer), "LOGOUT\n");
            write(sockfd, buffer, strlen(buffer));
            printf("\033[1;34mLogging out...\033[0m\n");
            printf("%s",thank);
            break;
        }

        else {
            printf("\033[1;31mInvalid option, try again.\033[0m\n");
        }
    }
    
    close(sockfd);
    return 0;
}
