#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

// The client_handler thread function.
void *client_handler(void *arg);

// Helper function to create a new client thread.
void create_client_thread(int *sockfd_ptr);

#endif // CLIENT_HANDLER_H
