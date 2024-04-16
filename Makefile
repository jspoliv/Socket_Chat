build: server client

server:
	gcc "./server.c" -o "./bin/server" -lws2_32 -lpthread

client:
	gcc "./client.c" -o "./bin/client" -lws2_32 -lpthread