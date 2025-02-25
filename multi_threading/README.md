# ChatApp

ChatApp is a multi-threaded chat application built in C that supports real-time messaging, file transfers, and persistent user management using MongoDB. The project includes both server and client components, with the client featuring a modern terminal-based user interface powered by ncurses.


![Screenshot from 2025-02-15 17-04-05](https://github.com/user-attachments/assets/9ea7847e-6019-4940-bb35-ffe2a66fd9c0)


## Features

- **User Registration & Login:**  
  Users can register and log in with a unique user ID.

- **Real-Time Messaging:**  
  Supports broadcast messages and private messaging (both one-shot and continuous sessions).

- **Chat History:**  
  Retrieve previous chat history between users.

- **Online User Listing:**  
  List currently online users as well as all registered users.

- **File Transfer:**  
  Transfer files between users seamlessly.

- **Cloud Data Persistence:**  
  All user information and chat data are stored in a cloud MongoDB database.

- **Offline Message Delivery:**  
  When users are offline, incoming messages are saved in the cloud MongoDB and automatically delivered when they log in, ensuring no message is lost.

- **Modular Design:**  
  The codebase is organized into separate modules for the server (client handling, database interactions, user management, and timeout handling) and client (UI, message receiving, and file transfers).

## Directory Structure



```plaintext
cnn/
├── chat_server/
│   ├── include/
│   │   ├── client_handler.h          // Function prototypes declared for client handling
│   │   │   void handle_client(int client_socket);
│   │   │   void create_client_thread(int *sockfd_ptr);
│   │   ├── db.h                      // Function prototypes for database operations
│   │   │   void init_mongo();
│   │   │   bool db_register_user(const char *username, const char *password, const char *ip_address, char *user_id_out, size_t user_id_size);
│   │   │   bool db_login_user(const char *username, const char *password, const char *ip_address, char *user_id_out, size_t user_id_size);
│   │   │   void db_set_user_offline(const char *user_id);
│   │   │   bool db_insert_chat(const char *sender_id, const char *recipient_id, const char *message, bool delivered, bool popped);
│   │   │   char* db_get_chat_history(const char *user_id, const char *other_id);
│   │   │   void deliver_offline_messages(const char *user_id, int sockfd);
│   │   ├── online_users.h            // Prototypes for online user management
│   │   │   void add_online_user(const char *username);
│   │   │   void remove_online_user(const char *username);
│   │   │   int get_online_sockfd(const char *user_id);
│   │   │   void broadcast_message(const char *sender_username, const char *message);
│   │   │   void get_online_users_list(char *buffer, size_t buf_size);
│   │   └── timeout.h                 // Prototypes for timeout handling
│   │       void start_timeout_checker();
│   ├── src/
│   │   ├── client_handler.c          // Function definitions for client handling
│   │   │   void *client_handler(void *arg) { … }
│   │   ├── db.c                      // Database operations definitions
│   │   ├── main.c                    // Main entry point for the chat server
│   │   ├── online_users.c            // Online user management functions
│   │   └── timeout.c                 // Timeout checker functions
│   └── Makefile                      // Build configuration for the server
├── chat_client/
│   ├── include/
│   │   ├── client.h                  // Prototype for client connection
│   │   ├── file_transfer.h           // Prototypes for file transfer functions
│   │   ├── receive_handler.h         // Prototypes for the receive handler thread/function
│   │   └── ui.h                      // Prototypes for UI display/update routines
│   ├── src/
│   │   ├── client_main.c             // Main entry point for the chat client
│   │   ├── file_transfer.c           // File transfer definitions
│   │   ├── receive_handler.c         // Receiving messages/files function
│   │   └── ui_ncurses.c              // User interface display function
│   └── Makefile                      // Build configuration for the client
└── .vscode/
    └── c_cpp_properties.json         // VS Code configuration for C/C++ 
```
## Tech Stack

**Client:**
- **Programming Language:** C
- **User Interface:** ncurses (for a modern, terminal-based UI)
- **Concurrency:** POSIX threads (pthreads)
- **Networking:** POSIX sockets (TCP)
- **Additional Libraries:** 
  - Standard C libraries (stdio, stdlib, string, etc.)
  - *(Planned)* OpenCV for future video call functionality

**Server:**
- **Programming Language:** C
- **Concurrency:** POSIX threads (pthreads)
- **Networking:** POSIX sockets (TCP)
- **Database:** MongoDB (using the MongoDB C Driver: mongoc and bson)
- **Data Persistence:** Cloud storage of user data, chat history, and offline messages
- **Additional Libraries:** Standard C libraries (stdio, stdlib, string, etc.)

## Demo

Insert gif or link to demo

## Environment Variables

To run this project, you will need the following environment variables:

- **MONGO_URI**  
  Your MongoDB connection string. For example:  
  `mongodb+srv://<username>:<password>@cluster0.mongodb.net/?retryWrites=true&w=majority`

- **MONGO_DB**  
  The name of the database to use. For example:  
  `chat`
# Installation

Before building the project, make sure the following external dependencies are installed:

### MongoDB C Driver

The application uses MongoDB to store user data and chat history. Install the MongoDB C Driver (which includes `mongoc` and `bson`). For example, on Ubuntu:

```bash
sudo apt-get update
sudo apt-get install libmongoc-1.0-dev libbson-1.0-dev
```

### Ncurses

The client’s terminal-based UI is built with ncurses. Install the ncurses development libraries. On Ubuntu, run:

```bash
 sudo apt-get install libncurses5-dev libncursesw5-dev
```
# Deployment

To deploy the project, you need to build and run both the server and client applications.


### Building and Running the Server

1. Open a terminal and navigate to the `cnn/chat_server` directory:
   ```bash
    cd cnn/chat_server
   ```
2. Build the server using the provided Makefile:
    ```bash 
     make
    ```
3. Run the server on your desired port (for example, 12345):
    ```bash
     ./chat_server 12345
    ```

### Building and Running the Server
 
1. Open a new terminal and navigate to the cnn/chat_client directory:
    ```bash
      cd cnn/chat_client
    ```
2. Build the client using the provided Makefile:
    ```bash 
     make
    ```
3. Run the client by specifying the server's IP address and the port used by the server (for example, connecting to localhost on port 12345):
    ```bash
      ./chat_client 127.0.0.1 12345
    ```
## Screenshots

![App Screenshot](https://via.placeholder.com/468x300?text=App+Screenshot+Here)


## Authors

- [@Rustam Kumar](https://github.com/Charlie-rk)
- [@Sangam kumar Mishra](https://github.com/sangam2ishra)
- [@Devanshu Dangi](https://github.com/devanshudangi)
- [@Parth Dodiya](https://github.com/Parthdodiya1230)
- [@Utkarsh Singh](https://github.com/Utkarshsingh90)


## Contributing

Contributions are always welcome!

See `contributing.md` for ways to get started.

Please adhere to this project's `code of conduct`.



