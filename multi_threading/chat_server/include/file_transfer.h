#ifndef FILE_TRANSFER_H
#define FILE_TRANSFER_H

#include <stdbool.h>

// Handles the file transfer command for a connected client.
// The sender’s socket (newsockfd) and username are passed in.
// This function reads additional command tokens (recipient, filename, filesize)
// from the current tokenization state. It returns true on success and false on failure.
bool handle_file_transfer(int newsockfd, const char *username);

#endif // FILE_TRANSFER_H
