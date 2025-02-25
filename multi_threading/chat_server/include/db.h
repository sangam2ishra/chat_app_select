#ifndef DB_H
#define DB_H

#include <stdbool.h>
#include <stddef.h>

// MongoDB initialization and cleanup.
void init_mongo();
void cleanup_mongo();

// User registration and login.
bool db_register_user(const char *username, const char *password, const char *ip_address,
                      char *user_id_out, size_t user_id_size);
bool db_login_user(const char *username, const char *password, const char *ip_address,
                   char *user_id_out, size_t user_id_size);
void db_set_user_offline(const char *user_id);

// Chat message functions.
bool db_insert_chat(const char *sender_id, const char *recipient_id, const char *message,bool delivered,bool popped);
char* db_get_chat_history(const char *user_id, const char *other_id);
// Returns a malloc()–allocated string containing all registered users (username : user_id).
// Caller must free() the returned string.
char* db_get_all_users(void);

void deliver_offline_messages(const char *user_id, int sockfd);


#endif // DB_H
