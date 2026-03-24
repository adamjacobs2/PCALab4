# Variables
CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = main
LIBS = -lm
SRC = main.c

# Default target: builds the executable
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

# Clean target: removes the executable
clean:
	rm -f $(TARGET)

# Run target: builds and then executes
run: all
	./$(TARGET)


