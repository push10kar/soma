CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
LDFLAGS = -lsqlite3
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
TARGET = soma

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/$(TARGET)
	sudo cp soma.bash /etc/bash_completion.d/soma
	@echo "installed soma → /usr/local/bin/soma"
	@echo "completions  → /etc/bash_completion.d/soma"
	@echo "run: source ~/.bashrc"

uninstall:
	rm -f /usr/local/bin/$(TARGET)
	sudo rm -f /etc/bash_completion.d/soma

.PHONY: all clean install uninstall