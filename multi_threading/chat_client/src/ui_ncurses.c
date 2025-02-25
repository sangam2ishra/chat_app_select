#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ui.h"
#include "client.h"
#include <time.h>


void print_menu() {
    printf("\n                  \033[1;36m**************** ============= Chat Menu ============= ****************\033[0m\n\n");
    printf("                                            \033[1;33m 1.\033[0m \033[1;32mSend Broadcast Message\033[0m\n");
    printf("                                            \033[1;33m 2.\033[0m \033[1;32mPrivate Chat Session (Continuous)\033[0m\n");
    printf("                                            \033[1;33m 3.\033[0m \033[1;32mRetrieve Chat History\033[0m\n");
    printf("                                            \033[1;33m 4.\033[0m \033[1;32mList Online Users\033[0m\n");
    printf("                                            \033[1;33m 5.\033[0m \033[1;32mList All Registered Users\033[0m\n");
    printf("                                            \033[1;33m 6.\033[0m \033[1;32mSend File\033[0m\n");
    printf("                                            \033[1;33m 7.\033[0m \033[1;31mLogout\033[0m\n");
    // printf("8. Video Call\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("                                            \033[5mEnter your choice:\033[0m ");
}


void authenticate(int sockfd, char *username, size_t username_size) {
    char option[10];
    char password[32];
    char user_id[32];
    char buffer[512];
    int choice;
    
    while (1) {
        printf("\n              ============================================   \033[1mWelcome to Chat Client\033[0m  ==========================================\n");
        printf("                                                  1. Register\n");
        printf("                                                  2. Login\n");
        printf("                                                  Enter choice: ");
        if (!fgets(option, sizeof(option), stdin))
            continue;
        choice = atoi(option);
        printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`\n");
        if (choice == 1) {
            printf("            Enter desired username : ");
            if (!fgets(username, username_size, stdin))
                continue;
            username[strcspn(username, "\n")] = '\0';
            printf("            Enter desired password : ");
            if (!fgets(password, sizeof(password), stdin))
                continue;
            password[strcspn(password, "\n")] = '\0';
            snprintf(buffer, sizeof(buffer), "REGISTER %s %s\n", username, password);
            write(sockfd, buffer, strlen(buffer));
            sleep(2);
            break;  // Assume auto‑login after registration.
        } else if (choice == 2) {
            printf("            Enter user_id: ");
            if (!fgets(user_id, username_size, stdin))
                continue;
            user_id[strcspn(user_id, "\n")] = '\0';
            printf("            Enter password: ");
            if (!fgets(password, sizeof(password), stdin))
                continue;
            password[strcspn(password, "\n")] = '\0';
            snprintf(buffer, sizeof(buffer), "LOGIN %s %s\n", user_id, password);
            write(sockfd, buffer, strlen(buffer));
            // username=user_id;
            strncpy(username,user_id,username_size-1);
            username[username_size-1]='\0';
              sleep(2);
            break;
        } else {
            printf("---------------------------  Invalid choice, try again. ---------------------------\n");
        }
    }
}
