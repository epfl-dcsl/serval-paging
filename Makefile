CFLAGS = -Wall -Wextra
DEBUG = -g
CC = gcc
INCLUDE = -Iinclude/
H_DIR = include/
C_DIR = src/
O_DIR = obj/
BIN = bin/
EXEC = main
SOURCES = $(wildcard $(C_DIR)*.c)
OBJECTS = $(patsubst $(C_DIR)%.c, $(O_DIR)%.o, $(SOURCES))
VERSION = v0.1
TARNAME = ServaledPaging-$(VERSION).tar.xz

LIBS = libjeu.a



$(BIN)$(EXEC): $(OBJECTS)
	@echo "-- LINKING OBJECTS --"
	@mkdir -p $(BIN)
	@$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

$(O_DIR)$(EXEC).o: $(C_DIR)$(EXEC).c   # Added because main.h doesn't exist
	@echo "Compiling $@"
	@mkdir -p $(O_DIR)
	@$(CC) $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@


$(O_DIR)%.o: $(C_DIR)%.c $(H_DIR)%.h
	@echo "Compiling $@"
	@mkdir -p $(O_DIR)
	@$(CC) $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@



clean:
	rm -fr $(BIN) $(O_DIR) $(TARNAME)

dist:
	tar -cv --lzma $(C_DIR) $(H_DIR) Makefile README.md -f $(TARNAME)
