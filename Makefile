NAME = ctimer
CC = gcc
CFLAGS = -Wall -O3 -DNOTIFY $(shell pkg-config --cflags libnotify)
LDFLAGS = $(shell pkg-config --libs libnotify)

all: main.o
	$(CC) -o $(NAME) $(LDFLAGS) main.o

install: $(NAME)
	cp $(NAME) ~/.local/bin/

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

clean:
	rm main.o $(NAME)
