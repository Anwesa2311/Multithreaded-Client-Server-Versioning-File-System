all: server client

server: Server.c
	gcc Server.c -o rfserver -lpthread

client: Client.c
	gcc Client.c -o rfs
