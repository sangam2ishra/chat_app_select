#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "online_users.h"

typedef struct OnlineUser {
    char user_id[128];
    char username[32];
    int sockfd;
    struct OnlineUser *next;
} OnlineUser;

static OnlineUser *online_users = NULL;
static pthread_mutex_t online_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_online_user(const char *user_id, const char *username, int sockfd) {
    pthread_mutex_lock(&online_mutex);
    OnlineUser *node = malloc(sizeof(OnlineUser));
    if (node) {
        strncpy(node->user_id, user_id, sizeof(node->user_id)-1);
        node->user_id[sizeof(node->user_id)-1] = '\0';
        strncpy(node->username, username, sizeof(node->username)-1);
        node->username[sizeof(node->username)-1] = '\0';
        node->sockfd = sockfd;
        node->next = online_users;
        online_users = node;
    }
    pthread_mutex_unlock(&online_mutex);
}

void remove_online_user(const char *user_id) {
    pthread_mutex_lock(&online_mutex);
    OnlineUser **curr = &online_users;
    while (*curr) {
        if (strcmp((*curr)->user_id, user_id) == 0) {
            OnlineUser *to_delete = *curr;
            *curr = (*curr)->next;
            free(to_delete);
            break;
        }
        curr = &((*curr)->next);
    }
    pthread_mutex_unlock(&online_mutex);
}

int get_online_sockfd(const char *user_id) {
    pthread_mutex_lock(&online_mutex);
    OnlineUser *curr = online_users;
    while (curr) {
        if (strcmp(curr->user_id, user_id) == 0) {
            int sock = curr->sockfd;
            pthread_mutex_unlock(&online_mutex);
            return sock;
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&online_mutex);
    return -1;
}

void broadcast_message(const char *sender_username, const char *message) {
    pthread_mutex_lock(&online_mutex);
    OnlineUser *curr = online_users;
    char full_message[1024];
    snprintf(full_message, sizeof(full_message), "[Broadcast from %s]: %s\n", sender_username, message);
    while (curr) {
        if (strcmp(curr->username, sender_username) != 0) {
            if (write(curr->sockfd, full_message, strlen(full_message)) < 0)
                perror("Error writing to socket");
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&online_mutex);
}


// New helper function to get a list of online users.
void get_online_users_list(char *buffer, size_t buf_size) {
    pthread_mutex_lock(&online_mutex);
    OnlineUser *curr = online_users;
    snprintf(buffer, buf_size, "Online users (username : user_id):\n");
    while (curr) {
        strncat(buffer, curr->username, buf_size - strlen(buffer) - 1);
        strncat(buffer, " : ", buf_size - strlen(buffer) - 1);
        strncat(buffer, curr->user_id, buf_size - strlen(buffer) - 1);
        strncat(buffer, "\n", buf_size - strlen(buffer) - 1);
        curr = curr->next;
    }
    pthread_mutex_unlock(&online_mutex);
}

