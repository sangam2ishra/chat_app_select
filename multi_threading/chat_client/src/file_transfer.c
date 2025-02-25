#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>   // for basename()
#include "file_transfer.h"
#include "client.h"
#include <dirent.h>
#include <time.h>
#include <limits.h>

#define TIMEOUT_SECONDS 30  // Timeout duration in seconds

void send_file(int sockfd) {
    // Debug: List files in current directory
    DIR *dir = opendir(".");
    if (dir) {
        struct dirent *entry;
        printf("DEBUG: Files in current directory:\n");
        while ((entry = readdir(dir)) != NULL) {
            printf("DEBUG: %s\n", entry->d_name);
        }
        closedir(dir);
    } else {
        perror("opendir");
    }

    // // Debug: Print current working directory
    // char cwd[PATH_MAX];
    // if (getcwd(cwd, sizeof(cwd)) != NULL) {
    //     printf("DEBUG: Current working directory: %s\n", cwd);
    // } else {
    //     perror("getcwd");
    // }

    char recipient[128];
    char filepath[256];
    char buffer[512];

    printf("Enter recipient user ID: ");
    if (!fgets(recipient, sizeof(recipient), stdin))
        return;
    recipient[strcspn(recipient, "\n")] = '\0';

    printf("Enter path to file: ");
    if (!fgets(filepath, sizeof(filepath), stdin))
        return;
    filepath[strcspn(filepath, "\n")] = '\0';

    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        perror("Error opening file");
        return;
    }
    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (filesize <= 0) {
        printf("File is empty or error determining file size.\n");
        fclose(fp);
        return;
    }
    char *file_buffer = malloc(filesize);
    if (!file_buffer) {
        fclose(fp);
        perror("Memory allocation error");
        return;
    }
    size_t bytes_read = fread(file_buffer, 1, filesize, fp);
    fclose(fp);
    if (bytes_read != filesize) {
        free(file_buffer);
        perror("Error reading file");
        return;
    }
    char filename[256];
    strcpy(filename, filepath);
    char *base = basename(filename);
    snprintf(buffer, sizeof(buffer), "FILE %s %s %ld\n", recipient, base, filesize);
    write(sockfd, buffer, strlen(buffer));

    size_t total_sent = 0;
    time_t start_time = time(NULL);
    while (total_sent < filesize) {
        // Check for timeout
        if (time(NULL) - start_time > TIMEOUT_SECONDS) {
            printf("\nTransfer taking too long. Cancelling file send.\n");
            break;
        }
        ssize_t bytes = write(sockfd, file_buffer + total_sent, filesize - total_sent);
        if (bytes < 0) {
            perror("Error sending file content");
            break;
        }
        total_sent += bytes;
        // Display progress loader
        double progress = ((double)total_sent / filesize) * 100;
        printf("\rSending file... %.2f%% complete", progress);
        fflush(stdout);
    }
    printf("\n");
    free(file_buffer);
}
