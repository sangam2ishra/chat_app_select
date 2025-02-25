// client.c
// A chat client for the multi-threaded Chat Server with file transfer support.
// Compile with: gcc -pthread client.c -o client
// Run with: ./client <server_ip> <port>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <libgen.h>  // for basename(), if available
#include <sys/stat.h>

#define MAX_BUFFER 512

int sockfd;  // global socket descriptor

// Thread function to continuously receive messages (and files) from the server.
void *receive_handler(void *arg) {
    char buffer[MAX_BUFFER];
    int n;
    while (1) {
        bzero(buffer, MAX_BUFFER);
        n = read(sockfd, buffer, MAX_BUFFER - 1);
        if (n <= 0) {
            printf("Disconnected from server.\n");
            exit(1);
        }
        buffer[n] = '\0';

        // Check if the message is a file header.
        if (strncmp(buffer, "FILE ", 5) == 0) {
            // Expected header format: "FILE <sender> <filename> <filesize>\n"
            char sender[32], filename[256];
            long filesize;
            int header_fields = sscanf(buffer, "FILE %31s %255s %ld", sender, filename, &filesize);
            if (header_fields != 3) {
                printf("Received malformed file header.\n");
            } else {
                // Find the newline to determine header length.
                char *newline = strchr(buffer, '\n');
                int header_length = newline ? (newline - buffer + 1) : n;
                int extra = n - header_length;  // bytes already read as part of file content
                char *file_buffer = malloc(filesize);
                if (!file_buffer) {
                    perror("Memory allocation error");
                    continue;
                }
                int total_read = 0;
                // If some file data was already read with the header, copy it.
                if (extra > 0) {
                    memcpy(file_buffer, buffer + header_length, extra);
                    total_read = extra;
                }
                // Read the remaining file content.
                while (total_read < filesize) {
                    int bytes = read(sockfd, file_buffer + total_read, filesize - total_read);
                    if (bytes <= 0) break;
                    total_read += bytes;
                }
                // Save the received file.
                char received_filename[300];
                snprintf(received_filename, sizeof(received_filename), "received_%s", filename);
                FILE *fp = fopen(received_filename, "wb");
                if (fp) {
                    fwrite(file_buffer, 1, filesize, fp);
                    fclose(fp);
                    printf("\nReceived file from %s, saved as %s\n", sender, received_filename);
                } else {
                    perror("Error saving received file");
                }
                free(file_buffer);
            }
        } else {
            // Normal text message.
            printf("\n%s\n", buffer);
        }
        printf(">> ");
        fflush(stdout);
    }
    return NULL;
}

// Function to display the main menu.
void print_menu() {
    printf("\n======== Chat Menu ========\n");
    printf("1. Send Broadcast Message\n");
    printf("2. Send Private Message\n");
    printf("3. List Online Users\n");
    printf("4. List All Registered Users\n");
    printf("5. Logout\n");
    printf("6. Send File\n");
    printf("===========================\n");
    printf("Enter your choice: ");
}

int main(int argc, char *argv[]) {
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

    char option[10];
    char username[32], password[32];
    char buffer[MAX_BUFFER];
    int choice;

    // First, prompt the user to register or login.
    while (1) {
        printf("\n====== Welcome to Chat Client ======\n");
        printf("1. Register\n");
        printf("2. Login\n");
        printf("Enter choice: ");
        if (!fgets(option, sizeof(option), stdin))
            continue;
        choice = atoi(option);
        if(choice == 1) {
            printf("Enter desired username: ");
            fgets(username, sizeof(username), stdin);
            username[strcspn(username, "\n")] = '\0';
            printf("Enter desired password: ");
            fgets(password, sizeof(password), stdin);
            password[strcspn(password, "\n")] = '\0';
            snprintf(buffer, sizeof(buffer), "REGISTER %s %s\n", username, password);
            write(sockfd, buffer, strlen(buffer));
            break;  // server auto-logs in after registration
        } else if(choice == 2) {
            printf("Enter username: ");
            fgets(username, sizeof(username), stdin);
            username[strcspn(username, "\n")] = '\0';
            printf("Enter password: ");
            fgets(password, sizeof(password), stdin);
            password[strcspn(password, "\n")] = '\0';
            snprintf(buffer, sizeof(buffer), "LOGIN %s %s\n", username, password);
            write(sockfd, buffer, strlen(buffer));
            break;
        } else {
            printf("Invalid choice, try again.\n");
        }
    }
    
    // Main loop: display menu and process user choices.
    while (1) {
        print_menu();
        if (!fgets(option, sizeof(option), stdin))
            continue;
        int menu_choice = atoi(option);
        if(menu_choice == 1) {  // Broadcast
            printf("Enter message to broadcast: ");
            if (!fgets(buffer, sizeof(buffer), stdin))
                continue;
            buffer[strcspn(buffer, "\n")] = '\0';
            char command[MAX_BUFFER];
            snprintf(command, sizeof(command), "BROADCAST %s\n", buffer);
            write(sockfd, command, strlen(command));
        }
        else if(menu_choice == 2) {  // Private message
            char recipient[32], message[MAX_BUFFER];
            printf("Enter recipient username: ");
            if (!fgets(recipient, sizeof(recipient), stdin))
                continue;
            recipient[strcspn(recipient, "\n")] = '\0';
            printf("Enter message: ");
            if (!fgets(message, sizeof(message), stdin))
                continue;
            message[strcspn(message, "\n")] = '\0';
            char command[MAX_BUFFER];
            snprintf(command, sizeof(command), "MSG %s %s\n", recipient, message);
            write(sockfd, command, strlen(command));
        }
        else if(menu_choice == 3) {  // List online users
            snprintf(buffer, sizeof(buffer), "LIST\n");
            write(sockfd, buffer, strlen(buffer));
        }
        else if(menu_choice == 4) {  // List all registered users
            snprintf(buffer, sizeof(buffer), "ALLUSERS\n");
            write(sockfd, buffer, strlen(buffer));
        }
        else if(menu_choice == 5) {  // Logout
            snprintf(buffer, sizeof(buffer), "LOGOUT\n");
            write(sockfd, buffer, strlen(buffer));
            printf("Logging out...\n");
            break;
        }
        else if(menu_choice == 6) {  // File transfer
            char recipient[32], filepath[256];
            printf("Enter recipient username: ");
            if (!fgets(recipient, sizeof(recipient), stdin))
                continue;
            recipient[strcspn(recipient, "\n")] = '\0';
            printf("Enter path to file: ");
            if (!fgets(filepath, sizeof(filepath), stdin))
                continue;
            filepath[strcspn(filepath, "\n")] = '\0';
            
            // Open file for reading in binary mode.
            FILE *fp = fopen(filepath, "rb");
            if (!fp) {
                perror("Error opening file");
                continue;
            }
            // Determine file size.
            fseek(fp, 0, SEEK_END);
            long filesize = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            if (filesize <= 0) {
                printf("File is empty or error determining file size.\n");
                fclose(fp);
                continue;
            }
            // Read file into a buffer.
            char *file_buffer = malloc(filesize);
            if (!file_buffer) {
                fclose(fp);
                perror("Memory allocation error");
                continue;
            }
            size_t bytes_read = fread(file_buffer, 1, filesize, fp);
            fclose(fp);
            if (bytes_read != filesize) {
                free(file_buffer);
                perror("Error reading file");
                continue;
            }
            // Extract filename (using basename if available).
            char filename[256];
            strcpy(filename, filepath);
            char *base = basename(filename); // Note: on some systems, you might need to implement your own.
            
            // Send file header: "FILE <recipient> <filename> <filesize>\n"
            snprintf(buffer, sizeof(buffer), "FILE %s %s %ld\n", recipient, base, filesize);
            write(sockfd, buffer, strlen(buffer));
            // Now send the file content.
            size_t total_sent = 0;
            while (total_sent < filesize) {
                ssize_t bytes = write(sockfd, file_buffer + total_sent, filesize - total_sent);
                if (bytes < 0) {
                    perror("Error sending file content");
                    break;
                }
                total_sent += bytes;
            }
            free(file_buffer);
        }
        else {
            printf("Invalid option, try again.\n");
        }
    }
    
    close(sockfd);
    return 0;
}
