#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mongoc/mongoc.h>
#include "db.h"

#define MAX_BUFFER 1024

// Global MongoDB objects.
mongoc_client_t *mongo_client = NULL;
mongoc_database_t *mongo_db = NULL;

static void generate_unique_user_id(const char *username, char *buffer, size_t size) {
    int random_number = (rand() % 900) + 100;
    snprintf(buffer, size, "%s_@%d", username, random_number);
}

void init_mongo() {
    mongoc_init();
    const char *uri_string = "mongodb+srv://rustampavri1275:ZEky8BjuetPZXF3B@cluster0.ycoqp.mongodb.net/?retryWrites=true&w=majority&appName=Cluster0";
    mongoc_uri_t *uri = mongoc_uri_new(uri_string);
    mongo_client = mongoc_client_new_from_uri(uri);
    mongoc_uri_destroy(uri);

    if (!mongo_client) {
        fprintf(stderr, "Failed to create MongoDB client\n");
        exit(EXIT_FAILURE);
    }
    mongo_db = mongoc_client_get_database(mongo_client, "chat");
}

void cleanup_mongo() {
    if (mongo_db) mongoc_database_destroy(mongo_db);
    if (mongo_client) mongoc_client_destroy(mongo_client);
    mongoc_cleanup();
}

bool db_register_user(const char *username, const char *password, const char *ip_address,
                      char *user_id_out, size_t user_id_size) {
    bson_error_t error;
    mongoc_collection_t *users_coll = mongoc_client_get_collection(mongo_client, "chat", "users");

    // Check for an existing username.
    bson_t *query = bson_new();
    BSON_APPEND_UTF8(query, "username", username);
    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(users_coll, query, NULL, NULL);
    const bson_t *doc = NULL;
    bool exists = mongoc_cursor_next(cursor, &doc);
    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    // if (exists) {
    //     mongoc_collection_destroy(users_coll);
    //     return false;
    // }

    generate_unique_user_id(username, user_id_out, user_id_size);

    bson_t *user_doc = bson_new();
    BSON_APPEND_UTF8(user_doc, "username", username);
    BSON_APPEND_UTF8(user_doc, "password", password);
    BSON_APPEND_UTF8(user_doc, "user_id", user_id_out);
    BSON_APPEND_BOOL(user_doc, "online", true);
    BSON_APPEND_UTF8(user_doc, "ip_address", ip_address);

    bool ret = mongoc_collection_insert_one(users_coll, user_doc, NULL, NULL, &error);
    if (!ret)
        fprintf(stderr, "Registration error: %s\n", error.message);

    bson_destroy(user_doc);
    mongoc_collection_destroy(users_coll);
    return ret;
}

bool db_login_user(const char *login_user_id, const char *password, const char *ip_address,
                   char *username, size_t username_size) {
    bson_error_t error;
    mongoc_collection_t *users_coll = mongoc_client_get_collection(mongo_client, "chat", "users");

    bson_t *query = bson_new();
    BSON_APPEND_UTF8(query, "user_id", login_user_id);
    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(users_coll, query, NULL, NULL);
    const bson_t *doc = NULL;
    bool found = mongoc_cursor_next(cursor, &doc);
    if (!found) {
        mongoc_cursor_destroy(cursor);
        bson_destroy(query);
        mongoc_collection_destroy(users_coll);
        return false;
    }
    
    bson_iter_t iter;
    if (bson_iter_init_find(&iter, doc, "password") && BSON_ITER_HOLDS_UTF8(&iter)) {
        const char *stored_pass = bson_iter_utf8(&iter, NULL);
        if (strcmp(stored_pass, password) != 0) {
            mongoc_cursor_destroy(cursor);
            bson_destroy(query);
            mongoc_collection_destroy(users_coll);
            return false;
        }
    }
    
    if (bson_iter_init_find(&iter, doc, "username") && BSON_ITER_HOLDS_UTF8(&iter)) {
        const char *stored_name = bson_iter_utf8(&iter, NULL);
        strncpy(username, stored_name, username_size-1);
        username[username_size-1] = '\0';
    }
    
    bson_t update;
    bson_t child;
    bson_init(&update);
    BSON_APPEND_DOCUMENT_BEGIN(&update, "$set", &child);
    BSON_APPEND_BOOL(&child, "online", true);
    BSON_APPEND_UTF8(&child, "ip_address", ip_address);
    bson_append_document_end(&update, &child);
    if (!mongoc_collection_update_one(users_coll, query, &update, NULL, NULL, &error))
        fprintf(stderr, "Login update error: %s\n", error.message);
    bson_destroy(&update);
    
    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    mongoc_collection_destroy(users_coll);
    return true;
}

void db_set_user_offline(const char *user_id) {
    bson_error_t error;
    mongoc_collection_t *users_coll = mongoc_client_get_collection(mongo_client, "chat", "users");
    bson_t *query = bson_new();
    BSON_APPEND_UTF8(query, "user_id", user_id);
    
    bson_t update;
    bson_t child;
    bson_init(&update);
    BSON_APPEND_DOCUMENT_BEGIN(&update, "$set", &child);
    BSON_APPEND_BOOL(&child, "online", false);
    bson_append_document_end(&update, &child);
    
    if (!mongoc_collection_update_one(users_coll, query, &update, NULL, NULL, &error))
        fprintf(stderr, "Error setting offline: %s\n", error.message);
    
    bson_destroy(&update);
    bson_destroy(query);
    mongoc_collection_destroy(users_coll);
}

bool db_insert_chat(const char *sender_id, const char *recipient_id, const char *message,bool delivered,bool popped) {
    bson_error_t error;
    mongoc_collection_t *chats_coll = mongoc_client_get_collection(mongo_client, "chat", "chats");
     int64_t now = (int64_t) time(NULL) * 1000;
    
    bson_t *chat_doc = bson_new();
    BSON_APPEND_UTF8(chat_doc, "sender_id", sender_id);
    BSON_APPEND_UTF8(chat_doc, "recipient_id", recipient_id);
    BSON_APPEND_DATE_TIME(chat_doc, "timestamp", (int64_t)time(NULL) * 1000);
    BSON_APPEND_UTF8(chat_doc, "message", message);
    BSON_APPEND_BOOL(chat_doc, "delivered", delivered);
    BSON_APPEND_INT64(chat_doc, "delivery_time", delivered ? now : 0);
    BSON_APPEND_INT64(chat_doc, "seen_time", 0);
    BSON_APPEND_BOOL(chat_doc, "popped", popped);
    
    bool ret = mongoc_collection_insert_one(chats_coll, chat_doc, NULL, NULL, &error);
    if (!ret)
        fprintf(stderr, "Error inserting chat: %s\n", error.message);
    bson_destroy(chat_doc);
    mongoc_collection_destroy(chats_coll);
    return ret;
}

char* db_get_chat_history(const char *user_id, const char *other_id) {
    mongoc_collection_t *chats_coll = mongoc_client_get_collection(mongo_client, "chat", "chats");
    
    bson_t *query = BCON_NEW("$or", "[",
                             "{", "sender_id", BCON_UTF8(user_id), "recipient_id", BCON_UTF8(other_id), "}",
                             "{", "sender_id", BCON_UTF8(other_id), "recipient_id", BCON_UTF8(user_id), "}",
                             "]");
    
    bson_t *opts = BCON_NEW("sort", "{", "timestamp", BCON_INT32(1), "}");
    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(chats_coll, query, opts, NULL);
    
    const bson_t *doc;
    size_t history_size = 4096;
    char *history = malloc(history_size);
    if (!history) {
        mongoc_cursor_destroy(cursor);
        bson_destroy(query);
        mongoc_collection_destroy(chats_coll);
        return NULL;
    }
    history[0] = '\0';
    char line[512];
    
  while (mongoc_cursor_next(cursor, &doc)) {
        bson_iter_t iter;
        const char *sender = "";
        int64_t ts = 0;
        const char *msg = "";
        int64_t dtime = 0, stime = 0;
        if (bson_iter_init_find(&iter, doc, "sender_id") && BSON_ITER_HOLDS_UTF8(&iter))
            sender = bson_iter_utf8(&iter, NULL);
        if (bson_iter_init_find(&iter, doc, "timestamp") && BSON_ITER_HOLDS_DATE_TIME(&iter))
            ts = bson_iter_date_time(&iter);
        if (bson_iter_init_find(&iter, doc, "message") && BSON_ITER_HOLDS_UTF8(&iter))
            msg = bson_iter_utf8(&iter, NULL);
        if (bson_iter_init_find(&iter, doc, "delivery_time") && BSON_ITER_HOLDS_INT64(&iter))
            dtime = bson_iter_int64(&iter);
        if (bson_iter_init_find(&iter, doc, "seen_time") && BSON_ITER_HOLDS_INT64(&iter))
            stime = bson_iter_int64(&iter);
            
        char time_str[26], delivered_str[26], seen_str[26];
        time_t t = ts / 1000;
        struct tm *tm_info = localtime(&t);
        strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        if (dtime != 0) {
            t = dtime / 1000;
            tm_info = localtime(&t);
            strftime(delivered_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        } else {
            strcpy(delivered_str, "-");
        }
        if (stime != 0) {
            t = stime / 1000;
            tm_info = localtime(&t);
            strftime(seen_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        } else {
            strcpy(seen_str, "-");
        }
        snprintf(line, sizeof(line), "[%s] %s: %s (Delivered: %s, Seen: %s)\n", time_str, sender, msg, delivered_str, seen_str);
        if (strlen(history) + strlen(line) + 1 > history_size) {
            history_size *= 2;
            history = realloc(history, history_size);
            if (!history)
                break;
        }
        strcat(history, line);
    }
    
    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    bson_destroy(opts);
    mongoc_collection_destroy(chats_coll);
    return history;
}


char* db_get_all_users(void) {
    mongoc_collection_t *users_coll = mongoc_client_get_collection(mongo_client, "chat", "users");
    bson_t *query = bson_new();
    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(users_coll, query, NULL, NULL);
    const bson_t *doc;
    
    size_t buf_size = 4096;
    char *list_buffer = malloc(buf_size);
    if (!list_buffer) {
        mongoc_cursor_destroy(cursor);
        bson_destroy(query);
        mongoc_collection_destroy(users_coll);
        return NULL;
    }
    strcpy(list_buffer, "All registered users (username : user_id):\n");
    
    while (mongoc_cursor_next(cursor, &doc)) {
        bson_iter_t iter;
        char uname[32] = "";
        char uid[128] = "";
        if (bson_iter_init_find(&iter, doc, "username") && BSON_ITER_HOLDS_UTF8(&iter))
            strncpy(uname, bson_iter_utf8(&iter, NULL), sizeof(uname) - 1);
        if (bson_iter_init_find(&iter, doc, "user_id") && BSON_ITER_HOLDS_UTF8(&iter))
            strncpy(uid, bson_iter_utf8(&iter, NULL), sizeof(uid) - 1);
        
        // Ensure there's enough space in the buffer.
        size_t needed = strlen(uname) + strlen(uid) + 10; // extra for formatting and newline.
        if (strlen(list_buffer) + needed >= buf_size) {
            buf_size *= 2;
            char *new_buffer = realloc(list_buffer, buf_size);
            if (!new_buffer) {
                free(list_buffer);
                list_buffer = NULL;
                break;
            }
            list_buffer = new_buffer;
        }
        strcat(list_buffer, uname);
        strcat(list_buffer, " : ");
        strcat(list_buffer, uid);
        strcat(list_buffer, "\n");
    }
    
    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    mongoc_collection_destroy(users_coll);
    
    return list_buffer;
}



// ---------------------
// Deliver Offline Messages
// ---------------------
// When a user logs in, deliver any messages stored while they were offline.
void deliver_offline_messages(const char *user_id, int sockfd) {
    mongoc_collection_t *chats_coll = mongoc_client_get_collection(mongo_client, "chat", "chats");
    bson_t *query = BCON_NEW("recipient_id", BCON_UTF8(user_id),
                              "popped", BCON_BOOL(false));
    bson_t *opts = BCON_NEW("sort", "{", "timestamp", BCON_INT32(1), "}");
    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(chats_coll, query, opts, NULL);
    const bson_t *doc;
    while (mongoc_cursor_next(cursor, &doc)) {
        bson_iter_t iter;
        const char *sender = "";
        int64_t ts = 0, dtime = 0, stime = 0;
        const char *msg = "";
        if (bson_iter_init_find(&iter, doc, "sender_id") && BSON_ITER_HOLDS_UTF8(&iter))
            sender = bson_iter_utf8(&iter, NULL);
        if (bson_iter_init_find(&iter, doc, "timestamp") && BSON_ITER_HOLDS_DATE_TIME(&iter))
            ts = bson_iter_date_time(&iter);
        if (bson_iter_init_find(&iter, doc, "message") && BSON_ITER_HOLDS_UTF8(&iter))
            msg = bson_iter_utf8(&iter, NULL);
        if (bson_iter_init_find(&iter, doc, "delivery_time") && BSON_ITER_HOLDS_INT64(&iter))
            dtime = bson_iter_int64(&iter);
        if (bson_iter_init_find(&iter, doc, "seen_time") && BSON_ITER_HOLDS_INT64(&iter))
            stime = bson_iter_int64(&iter);
        
        char sent_time[26], delivered_time[26], seen_time[26];
        time_t t = ts / 1000;
        struct tm *tm_info = localtime(&t);
        strftime(sent_time, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        if (dtime != 0) {
            t = dtime / 1000;
            tm_info = localtime(&t);
            strftime(delivered_time, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        } else {
            strcpy(delivered_time, "-");
        }
        if (stime != 0) {
            t = stime / 1000;
            tm_info = localtime(&t);
            strftime(seen_time, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        } else {
            strcpy(seen_time, "-");
        }
        char offline_msg[MAX_BUFFER];
        snprintf(offline_msg, sizeof(offline_msg), "[%s] %s: %s (Delivered: %s, Seen: %s)\n", sent_time, sender, msg, delivered_time, seen_time);
        send(sockfd, offline_msg, strlen(offline_msg), 0);

        // Mark this message as popped and update delivery_time.
        bson_t *update = BCON_NEW("$set", "{", "popped", BCON_BOOL(true), "delivered", BCON_BOOL(true), "delivery_time", BCON_INT64((int64_t) time(NULL) * 1000), "}");
        bson_iter_t id_iter;
        if (bson_iter_init_find(&id_iter, doc, "_id") && BSON_ITER_HOLDS_OID(&id_iter)) {
            const bson_oid_t *oid = bson_iter_oid(&id_iter);
            bson_t *doc_query = BCON_NEW("_id", BCON_OID(oid));
            bson_error_t error;
            mongoc_collection_update_one(chats_coll, doc_query, update, NULL, NULL, &error);
            bson_destroy(doc_query);
        }
        bson_destroy(update);
    }
    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
    bson_destroy(opts);
    mongoc_collection_destroy(chats_coll);
}