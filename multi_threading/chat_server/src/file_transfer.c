#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "file_transfer.h"
#include "online_users.h"

#define MAX_BUFFER 1024

bool handle_file_transfer(int newsockfd, const char *username) {
    // Get additional parameters from the command using strtok.
    char *recipient_id = strtok(NULL, " ");
    char *filename = strtok(NULL, " ");
    char *filesize_str = strtok(NULL, " ");
    printf("Hii");
    
    if (!recipient_id || !filename || !filesize_str) {
        send(newsockfd, "Usage: FILE <recipient_user_id> <filename> <filesize>\n", 55, 0);
        return false;
    }
    
    long filesize = atol(filesize_str);
    if (filesize <= 0) {
        send(newsockfd, "Invalid file size.\n", 21, 0);
        return false;
    }
    
    char *file_buffer = malloc(filesize);
    if (!file_buffer) {
        send(newsockfd, "Memory allocation error.\n", 27, 0);
        return false;
    }
    
    size_t total_read = 0;
    while (total_read < filesize) {
        ssize_t bytes = read(newsockfd, file_buffer + total_read, filesize - total_read);
        if (bytes <= 0) {
            send(newsockfd, "Error reading file content.\n", 29, 0);
            free(file_buffer);
            return false;
        }
        total_read += bytes;
    }
    
    int recipient_sock = get_online_sockfd(recipient_id);
    if (recipient_sock == -1) {
        send(newsockfd, "Recipient not available.\n", 25, 0);
        free(file_buffer);
        return false;
    }
    
    char header[MAX_BUFFER];
    snprintf(header, sizeof(header), "FILE %s %s %ld\n", username, filename, filesize);
    send(recipient_sock, header, strlen(header), 0);
    
    size_t total_sent = 0;
    while (total_sent < filesize) {
        ssize_t bytes = write(recipient_sock, file_buffer + total_sent, filesize - total_sent);
        if (bytes < 0) {
            perror("Error sending file content to recipient");
            break;
        }
        total_sent += bytes;
    }
    free(file_buffer);
    
    send(newsockfd, "File sent successfully.\n", 26, 0);
    return true;
}
