#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>  // for basename()
#include <arpa/inet.h>
#include "receive_handler.h"
#include "client.h"

void *receive_handler(void *arg) {
    char buffer[MAX_BUFFER];
    int n;
    while (1) {
        memset(buffer, 0, MAX_BUFFER);
        n = read(sockfd, buffer, MAX_BUFFER - 1);
        if (n <= 0) {
            printf("Disconnected from server.\n");
            exit(1);
        }
        buffer[n] = '\0';

        // Check if the message is a file header.
        if (strncmp(buffer, "FILE ", 5) == 0) {
            // Expected header format: "FILE <sender_username> <filename> <filesize>\n"
            char sender[32], filename[256];
            long filesize;
            int header_fields = sscanf(buffer, "FILE %31s %255s %ld", sender, filename, &filesize);
            if (header_fields != 3) {
                printf("Received malformed file header.\n");
            } else {
                char *newline = strchr(buffer, '\n');
                int header_length = newline ? (newline - buffer + 1) : n;
                int extra = n - header_length;  // bytes already read as file data
                char *file_buffer = malloc(filesize);
                if (!file_buffer) {
                    perror("Memory allocation error");
                    continue;
                }
                int total_read = 0;
                if (extra > 0) {
                    memcpy(file_buffer, buffer + header_length, extra);
                    total_read = extra;
                }
                while (total_read < filesize) {
                    int bytes = read(sockfd, file_buffer + total_read, filesize - total_read);
                    if (bytes <= 0) break;
                    total_read += bytes;
                }
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
