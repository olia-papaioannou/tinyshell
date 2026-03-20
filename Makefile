CC = gcc
CFLAGS = -Wall
LIBS = -lreadline
TARGET = tinyshell
SRC = tinyshell.c

all: build

build:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

run: build
	./$(TARGET)

clean:
	rm -f $(TARGET)
