CC=gcc
CFLAGS=-Wall -Wextra -Werror

all: udp_server udp_client

udp_server: udp_server.c queue.o util.o
	$(CC) $(CFLAGS) -o udp_server udp_server.c queue.o util.o

udp_client: udp_client.c queue.o util.o
	$(CC) $(CFLAGS) -o udp_client udp_client.c queue.o util.o

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c

util.o: util.c util.h
	$(CC) $(CFLAGS) -c util.c

run: udp_server udp_client
	gnome-terminal -- bash -c "./udp_server 1600; exec bash" &
	gnome-terminal -- bash -c "./udp_client localhost 1600; exec bash"

push:
ifndef COMMIT_MSG
	$(error COMMIT_MSG is not set)
endif
	git add .
	git commit -m "$(COMMIT_MSG)"
	git push

clean:
	rm -f udp_server udp_client queue.o util.o



