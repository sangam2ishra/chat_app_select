#ifndef UI_H
#define UI_H

#include <stddef.h> // For size_t

// Displays the main menu.
void print_menu();

// Prompts the user for registration or login and sends the appropriate command
// to the server. The authenticated username is returned in the 'username' parameter.
void authenticate(int sockfd, char *username, size_t username_size);

#endif // UI_H
