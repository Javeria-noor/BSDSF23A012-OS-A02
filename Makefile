CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
SRC = src/ls-v1.1.0.c
BINDIR = bin
TARGET = $(BINDIR)/ls

.PHONY: all clean

all: $(TARGET)

$(BINDIR):
	mkdir -p $(BINDIR)

$(TARGET): $(SRC) | $(BINDIR)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -rf $(BINDIR)
