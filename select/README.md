
---

## Detailed Explanation

### 1. Select()-Based Chat Server with Idle Timeout

- **Architecture:**  
  This version uses a single-threaded event loop driven by the `select()` system call to monitor the listening socket and all active client sockets. A master set of file descriptors is maintained and periodically checked for new data and idle timeouts.

- **Features:**
  - **Registration and Login:**  
    Supports the same `REGISTER` and `LOGIN` commands as the multi-threaded version.
  - **Messaging and File Transfer:**  
    Provides commands for sending private messages (`MSG`), broadcasting (`BROADCAST`), and transferring files (`FILE`).
  - **User Listing:**  
    Clients can request a list of online users (`LIST`) or all registered users (`ALLUSERS`).
  - **Idle Timeout:**  
    The server sets a 1-second timeout on the `select()` call and checks each client's `last_active` timestamp after every cycle. If a client has been idle for more than 180 seconds, it is disconnected automatically.
  - **Logout:**  
    Clients can use the `LOGOUT` command to disconnect.

- **Scalability:**  
  This event-driven design avoids the overhead of thread creation and context switching, making it suitable for handling a moderate number of simultaneous connections efficiently.

---

### 2. Chat Client

- **User Interface:**  
  The client is terminal-based and provides an initial menu for registration/login, followed by a main menu that includes:
  - **Sending Broadcast Messages**
  - **Sending Private Messages**
  - **Listing Online Users**
  - **Listing All Registered Users**
  - **Sending Files**  
    (When you select the file transfer option, you are prompted for the recipient's username and the file path.)
  - **Logout**

- **Message Reception:**  
  A dedicated receiver thread continuously listens for messages from the server and displays them. This ensures incoming messages (including file transfer headers and data) do not interfere with your input.

- **File Transfer in the Client:**  
  - **Sending Files:**  
    When you choose the file transfer option (for example, option 6), you will be prompted to enter:
    1. The recipient's username.
    2. The file path (if your file, e.g., `sangam.txt`, is in the same directory as the client executable, simply enter `sangam.txt`).
    
    The client then reads the file, determines its size, and sends a header along with the file contents to the server.
  - **Receiving Files:**  
    When a file is sent to you, your client detects the header (which starts with `FILE`), reads the specified number of bytes for the file, and saves it (typically prefixed with `received_`).

---




