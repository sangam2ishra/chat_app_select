CC = gcc
CFLAGS = -Wall -Wextra -pthread 
INC = -Iinclude
SRC = src/client_main.c src/receive_handler.c src/ui_ncurses.c src/file_transfer.c
TARGET = chat_client

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INC) $(SRC) -o $(TARGET) -lncurses

clean:
	rm -f $(TARGET)