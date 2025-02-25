#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "db.h"
#include "client_handler.h"
#include "timeout.h"
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
         fprintf(stderr, "Usage: %s <port>\n", argv[0]);
         exit(EXIT_FAILURE);
    }
    int portno = atoi(argv[1]);
    srand(time(NULL));

    // Initialize MongoDB.
    init_mongo();

    int sockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen = sizeof(cli_addr);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
         perror("Error opening socket");
         exit(EXIT_FAILURE);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
         perror("Error on binding");
         exit(EXIT_FAILURE);
    }
    listen(sockfd, 5);
    printf("Chat server listening on port %d\n", portno);

    // Start the timeout checker thread.
    start_timeout_checker();

    

    while (1) {
         int *newsockfd_ptr = malloc(sizeof(int));
         if (!newsockfd_ptr) {
             perror("Memory allocation error");
             continue;
         }
         *newsockfd_ptr = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
         if (*newsockfd_ptr < 0) {
             perror("Error on accept");
             free(newsockfd_ptr);
             continue;
         }
         create_client_thread(newsockfd_ptr);
    }

    close(sockfd);
    cleanup_mongo();
    return 0;
}
