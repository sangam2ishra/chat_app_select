CC = gcc
CFLAGS = -Wall -Wextra -pthread `pkg-config --cflags libmongoc-1.0`
INC = -Iinclude
SRC = src/main.c src/db.c src/client_handler.c src/online_users.c src/timeout.c src/file_transfer.c
TARGET = chat_server
LIBS = `pkg-config --libs libmongoc-1.0`
TARGET = chat_server

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INC) $(SRC) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)
