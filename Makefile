server:
	gcc "./Server.c" -o "./bin/Server.exe" -lws2_32 -lpthread

client:
	gcc "./Client.c" -o "./bin/Client.exe" -lws2_32 -lpthread

run: server client