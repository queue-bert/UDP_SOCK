CC=gcc
CFLAGS=-Wall -Wextra -Werror

all: udp_server udp_client

udp_server: udp_server.c
	$(CC) $(CFLAGS) -o udp_server udp_server.c

udp_client: udp_client.c
	$(CC) $(CFLAGS) -o udp_client udp_client.c

run: udp_server udp_client
	gnome-terminal -- bash -c "./udp_server 1600; exec bash" &
	gnome-terminal -- bash -c "./udp_client localhost 1600; exec bash"

clean:
	rm -f udp_server udp_client

