#ifndef ONLINE_USERS_H
#define ONLINE_USERS_H

// Add, remove, and lookup online users.
void add_online_user(const char *user_id, const char *username, int sockfd);
void remove_online_user(const char *user_id);
int get_online_sockfd(const char *user_id);

// Broadcast a message to all online users (except the sender).
void broadcast_message(const char *sender_username, const char *message);
// New function declaration:
void get_online_users_list(char *buffer, size_t buf_size);

#endif // ONLINE_USERS_H
