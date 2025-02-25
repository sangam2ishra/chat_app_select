// server_select.c
// A select()-based Chat Server with an in-memory user database, file transfer,
// and an inactivity timeout (5 minutes).
// Compile with: gcc server_select.c -o server_select
// Run with: ./server_select <port>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/select.h>

#define MAX_BUFFER 512
#define TIMEOUT_SECONDS 300  // Inactivity timeout (5 minutes)

// --------------------
// User database (single-threaded, so no mutexes are needed)
// --------------------
typedef struct User {
    char username[32];
    char password[32];
    char ip_address[INET_ADDRSTRLEN];
    int sockfd; // valid if online, -1 otherwise
    time_t last_active;
    struct User *next;
} User;

User *user_list = NULL;

void error(const char *msg) {
    perror(msg);
    exit(1);
}

User* find_user(const char *username) {
    User *curr = user_list;
    while (curr != NULL) {
        if (strcmp(curr->username, username) == 0)
            return curr;
        curr = curr->next;
    }
    return NULL;
}

int add_user(const char *username, const char *password, const char *ip_address, int sockfd) {
    if (find_user(username) != NULL) {
        return -1; // already exists
    }
    User *new_user = malloc(sizeof(User));
    if (!new_user) {
        return -2; // memory error
    }
    strncpy(new_user->username, username, sizeof(new_user->username)-1);
    new_user->username[sizeof(new_user->username)-1] = '\0';
    strncpy(new_user->password, password, sizeof(new_user->password)-1);
    new_user->password[sizeof(new_user->password)-1] = '\0';
    strncpy(new_user->ip_address, ip_address, INET_ADDRSTRLEN-1);
    new_user->ip_address[INET_ADDRSTRLEN-1] = '\0';
    new_user->sockfd = sockfd;
    new_user->last_active = time(NULL);
    new_user->next = user_list;
    user_list = new_user;
    return 0;
}

void update_user_online(const char *username, const char *ip_address, int sockfd) {
    User *user = find_user(username);
    if (user != NULL) {
        strncpy(user->ip_address, ip_address, INET_ADDRSTRLEN-1);
        user->ip_address[INET_ADDRSTRLEN-1] = '\0';
        user->sockfd = sockfd;
        user->last_active = time(NULL);
    }
}

void set_user_offline(const char *username) {
    User *user = find_user(username);
    if (user != NULL) {
        user->sockfd = -1;
    }
}

// --------------------
// Messaging functions
// --------------------
void send_message_to_client(int sockfd, const char *message) {
    if (write(sockfd, message, strlen(message)) < 0)
         perror("ERROR writing to socket");
}

void broadcast_message(const char *sender, const char *message) {
    User *curr = user_list;
    char full_message[MAX_BUFFER];
    snprintf(full_message, sizeof(full_message), "[%s]: %s\n", sender, message);
    while (curr != NULL) {
        if (curr->sockfd != -1 && strcmp(curr->username, sender) != 0)
            send_message_to_client(curr->sockfd, full_message);
        curr = curr->next;
    }
}

void send_private_message(const char *sender, const char *recipient, const char *message) {
    User *user = find_user(recipient);
    if (user == NULL || user->sockfd == -1)
        return;
    char full_message[MAX_BUFFER];
    snprintf(full_message, sizeof(full_message), "[PM from %s]: %s\n", sender, message);
    send_message_to_client(user->sockfd, full_message);
}

void list_online_users(int sockfd) {
    User *curr = user_list;
    char list_buffer[MAX_BUFFER] = "Online users:\n";
    while (curr != NULL) {
        if (curr->sockfd != -1) {
            strcat(list_buffer, curr->username);
            strcat(list_buffer, "\n");
        }
        curr = curr->next;
    }
    send_message_to_client(sockfd, list_buffer);
}

void list_all_registered_users(int sockfd) {
    User *curr = user_list;
    char list_buffer[MAX_BUFFER] = "Registered users:\n";
    while (curr != NULL) {
        strcat(list_buffer, curr->username);
        strcat(list_buffer, "\n");
        curr = curr->next;
    }
    send_message_to_client(sockfd, list_buffer);
}

// --------------------
// Client structure for select-based server
// --------------------
typedef struct Client {
    int sockfd;
    struct sockaddr_in addr;
    char username[32];  // set after login/registration
    int logged_in;      // 0 = not logged in, 1 = logged in
    time_t last_active;
    char buffer[MAX_BUFFER];
    struct Client *next;
} Client;

Client *client_list = NULL;

void add_client(int sockfd, struct sockaddr_in addr) {
    Client *new_client = malloc(sizeof(Client));
    if (!new_client) {
        perror("Memory allocation error");
        return;
    }
    new_client->sockfd = sockfd;
    new_client->addr = addr;
    new_client->logged_in = 0;
    new_client->username[0] = '\0';
    new_client->last_active = time(NULL);
    memset(new_client->buffer, 0, MAX_BUFFER);
    new_client->next = client_list;
    client_list = new_client;
}

void remove_client(int sockfd) {
    Client **curr = &client_list;
    while (*curr) {
        Client *entry = *curr;
        if (entry->sockfd == sockfd) {
            *curr = entry->next;
            close(entry->sockfd);
            free(entry);
            return;
        }
        curr = &entry->next;
    }
}

// Helper: send welcome message to a new client.
void send_welcome_message(int sockfd) {
    char *welcome =
        "Welcome to Chat Server!\n"
        "Commands:\n"
        "  REGISTER <username> <password>\n"
        "  LOGIN <username> <password>\n"
        "  MSG <recipient> <message>\n"
        "  BROADCAST <message>\n"
        "  FILE <recipient> <filename> <filesize>   (followed by file content)\n"
        "  LIST (online users)\n"
        "  ALLUSERS (all registered users)\n"
        "  LOGOUT\n";
    send_message_to_client(sockfd, welcome);
}

// --------------------
// Main select()-based server loop
// --------------------
int main(int argc, char *argv[]) {
    int listenfd, portno;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    
    if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
    }
    portno = atoi(argv[1]);
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
        error("ERROR opening socket");
    
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    
    if (bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    
    listen(listenfd, 5);
    printf("Select-based Chat server listening on port %d\n", portno);
    
    fd_set readfds;
    int maxfd;
    struct timeval tv;
    
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(listenfd, &readfds);
        maxfd = listenfd;
        
        // Add all client sockets to the set.
        Client *curr_client = client_list;
        while (curr_client) {
            FD_SET(curr_client->sockfd, &readfds);
            if (curr_client->sockfd > maxfd)
                maxfd = curr_client->sockfd;
            curr_client = curr_client->next;
        }
        
        // Set select timeout to 1 second to allow periodic timeout checks.
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        int activity = select(maxfd + 1, &readfds, NULL, NULL, &tv);
        if (activity < 0) {
            perror("select error");
            continue;
        }
        
        // Check for new connection.
        if (FD_ISSET(listenfd, &readfds)) {
            int newsockfd = accept(listenfd, (struct sockaddr *) &cli_addr, &clilen);
            if (newsockfd < 0) {
                perror("ERROR on accept");
            } else {
                add_client(newsockfd, cli_addr);
                send_welcome_message(newsockfd);
                printf("New connection from %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            }
        }
        
        // Check all clients for incoming data.
        curr_client = client_list;
        while (curr_client) {
            int sd = curr_client->sockfd;
            Client *next_client = curr_client->next;  // in case we remove this client
            if (FD_ISSET(sd, &readfds)) {
                memset(curr_client->buffer, 0, MAX_BUFFER);
                int n = read(sd, curr_client->buffer, MAX_BUFFER - 1);
                if (n <= 0) {
                    printf("Client disconnected (sockfd: %d)\n", sd);
                    if (curr_client->logged_in) {
                        set_user_offline(curr_client->username);
                    }
                    remove_client(sd);
                } else {
                    curr_client->buffer[n] = '\0';
                    curr_client->last_active = time(NULL);
                    
                    // Tokenize the input. Note that for file transfers the header
                    // and file content might be combined in one read.
                    char *command = strtok(curr_client->buffer, " \n");
                    if (command == NULL) {
                        // empty input, ignore.
                    }
                    else if (strcasecmp(command, "REGISTER") == 0) {
                        char *reg_username = strtok(NULL, " \n");
                        char *reg_password = strtok(NULL, "\n");
                        if (!reg_username || !reg_password) {
                            send_message_to_client(sd, "Usage: REGISTER <username> <password>\n");
                        } else {
                            char client_ip[INET_ADDRSTRLEN];
                            inet_ntop(AF_INET, &(curr_client->addr.sin_addr), client_ip, INET_ADDRSTRLEN);
                            int res = add_user(reg_username, reg_password, client_ip, sd);
                            if (res == 0) {
                                strncpy(curr_client->username, reg_username, sizeof(curr_client->username)-1);
                                curr_client->logged_in = 1;
                                send_message_to_client(sd, "Registration successful. You are now logged in.\n");
                            } else {
                                send_message_to_client(sd, "Registration failed. Username may already exist.\n");
                            }
                        }
                    }
                    else if (strcasecmp(command, "LOGIN") == 0) {
                        char *login_username = strtok(NULL, " \n");
                        char *login_password = strtok(NULL, "\n");
                        if (!login_username || !login_password) {
                            send_message_to_client(sd, "Usage: LOGIN <username> <password>\n");
                        } else {
                            User *user = find_user(login_username);
                            if (!user) {
                                send_message_to_client(sd, "User not found. Please register first.\n");
                            } else {
                                if (strcmp(user->password, login_password) == 0) {
                                    strncpy(curr_client->username, login_username, sizeof(curr_client->username)-1);
                                    curr_client->logged_in = 1;
                                    char client_ip[INET_ADDRSTRLEN];
                                    inet_ntop(AF_INET, &(curr_client->addr.sin_addr), client_ip, INET_ADDRSTRLEN);
                                    update_user_online(login_username, client_ip, sd);
                                    send_message_to_client(sd, "Login successful.\n");
                                } else {
                                    send_message_to_client(sd, "Incorrect password.\n");
                                }
                            }
                        }
                    }
                    else if (strcasecmp(command, "MSG") == 0) {
                        if (!curr_client->logged_in) {
                            send_message_to_client(sd, "Please login first.\n");
                        } else {
                            char *recipient = strtok(NULL, " ");
                            char *msg = strtok(NULL, "\n");
                            if (!recipient || !msg) {
                                send_message_to_client(sd, "Usage: MSG <recipient> <message>\n");
                            } else {
                                send_private_message(curr_client->username, recipient, msg);
                            }
                        }
                    }
                    else if (strcasecmp(command, "BROADCAST") == 0) {
                        if (!curr_client->logged_in) {
                            send_message_to_client(sd, "Please login first.\n");
                        } else {
                            char *msg = strtok(NULL, "\n");
                            if (!msg) {
                                send_message_to_client(sd, "Usage: BROADCAST <message>\n");
                            } else {
                                broadcast_message(curr_client->username, msg);
                            }
                        }
                    }
                    else if (strcasecmp(command, "FILE") == 0) {
                        // File transfer command.
                        if (!curr_client->logged_in) {
                            send_message_to_client(sd, "Please login first.\n");
                        } else {
                            char *recipient = strtok(NULL, " ");
                            char *filename = strtok(NULL, " ");
                            char *filesize_str = strtok(NULL, " ");
                            if (!recipient || !filename || !filesize_str) {
                                send_message_to_client(sd, "Usage: FILE <recipient> <filename> <filesize>\n");
                            } else {
                                long filesize = atol(filesize_str);
                                if (filesize <= 0) {
                                    send_message_to_client(sd, "Invalid file size.\n");
                                } else {
                                    // Determine if some file content was already read.
                                    char *newline = strchr(curr_client->buffer, '\n');
                                    int header_length = newline ? (newline - curr_client->buffer + 1) : n;
                                    int extra = n - header_length;
                                    char *file_buffer = malloc(filesize);
                                    if (!file_buffer) {
                                        send_message_to_client(sd, "Memory allocation error.\n");
                                    } else {
                                        int total_read = 0;
                                        if (extra > 0) {
                                            memcpy(file_buffer, curr_client->buffer + header_length, extra);
                                            total_read = extra;
                                        }
                                        // Read the remaining file content.
                                        while (total_read < filesize) {
                                            int bytes = read(sd, file_buffer + total_read, filesize - total_read);
                                            if (bytes <= 0)
                                                break;
                                            total_read += bytes;
                                        }
                                        // Look up the recipient.
                                        User *user = find_user(recipient);
                                        if (user == NULL || user->sockfd == -1) {
                                            send_message_to_client(sd, "Recipient not available.\n");
                                            free(file_buffer);
                                        } else {
                                            char header[MAX_BUFFER];
                                            snprintf(header, sizeof(header), "FILE %s %s %ld\n", curr_client->username, filename, filesize);
                                            send_message_to_client(user->sockfd, header);
                                            size_t total_sent = 0;
                                            while (total_sent < filesize) {
                                                ssize_t bytes = write(user->sockfd, file_buffer + total_sent, filesize - total_sent);
                                                if (bytes < 0) {
                                                    perror("Error sending file content");
                                                    break;
                                                }
                                                total_sent += bytes;
                                            }
                                            send_message_to_client(sd, "File sent successfully.\n");
                                            free(file_buffer);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (strcasecmp(command, "LIST") == 0) {
                        if (!curr_client->logged_in) {
                            send_message_to_client(sd, "Please login first.\n");
                        } else {
                            list_online_users(sd);
                        }
                    }
                    else if (strcasecmp(command, "ALLUSERS") == 0) {
                        list_all_registered_users(sd);
                    }
                    else if (strcasecmp(command, "LOGOUT") == 0) {
                        send_message_to_client(sd, "Logging out. Goodbye!\n");
                        if (curr_client->logged_in)
                            set_user_offline(curr_client->username);
                        remove_client(sd);
                    }
                    else {
                        send_message_to_client(sd, "Unknown command.\n");
                    }
                }
            }
            curr_client = next_client;
        }
        
        // Periodically check for inactivity.
        time_t now = time(NULL);
        curr_client = client_list;
        while (curr_client) {
            Client *next_client = curr_client->next;
            if (difftime(now, curr_client->last_active) > TIMEOUT_SECONDS) {
                send_message_to_client(curr_client->sockfd, "You have been disconnected due to inactivity.\n");
                if (curr_client->logged_in)
                    set_user_offline(curr_client->username);
                printf("Client timed out (sockfd: %d)\n", curr_client->sockfd);
                remove_client(curr_client->sockfd);
            }
            curr_client = next_client;
        }
    }
    
    close(listenfd);
    return 0;
}
