#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <pthread.h>

#define BACKLOG 10
#define BUFSIZE 1024

typedef struct Server {
    int prev, client, exit;
    SOCKET serverSocket;
    SOCKET sockets[BACKLOG];
    pthread_t th[BACKLOG];
    pthread_mutex_t lock;
} server;

// Returns an AF_INET SOCKARR_IN at ip:port
SOCKADDR_IN newAddress(char* ip, int port);

// Returns a server with starting values
server newServer(SOCKET serverSocket, int size);

// Returns the position of the first available socket on the list of length size
int notFull(SOCKET* list, int size);

void acceptLoop(SOCKET serverSocket);

void* recvLoop(void* pServer);

// Sends the recv message to other sockets in the server
void resend(char* buffer, int sender, server* s); 


int main() {
    WSADATA WSAData;
    SOCKET serverSocket;
    SOCKADDR_IN serverAddress;

    WSAStartup(MAKEWORD(2,0), &WSAData);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket == INVALID_SOCKET) {
        printf("[socket() failed]\n\n");
        WSACleanup();
        system("pause");
        exit(1);
    }
    printf("[socket() resolved successfully]\n");

    serverAddress = newAddress("127.0.0.1", 3000);

    if(bind(serverSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) != 0) {
        printf("[bind() failed]\n\n");
        closesocket(serverSocket);
        WSACleanup();
        system("pause");
        exit(1);
    }
    printf("[bind() resolved successfully]\n");


    if(listen(serverSocket, BACKLOG) != 0){
        printf("[listen() failed]\n\n");
        closesocket(serverSocket);
        WSACleanup();
        system("pause");
        exit(1);
    }
    printf("[listen() resolved successfully]\n");

    acceptLoop(serverSocket);

    shutdown(serverSocket, SD_BOTH);
    closesocket(serverSocket);
    WSACleanup();
    printf("\n\n");
    system("pause");
    return 0;
}

SOCKADDR_IN newAddress(char* ip, int port) {
    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = (ip == NULL || strlen(ip) == 0) ? INADDR_ANY : inet_addr(ip) ;
    return addr;
}

server newServer(SOCKET serverSocket, int size) {
    server s;
    s.serverSocket = serverSocket;
    s.lock = PTHREAD_MUTEX_INITIALIZER;
    s.prev = -1;
    s.exit = 0;
    s.client = -1;
    for (int i=0; i<size; i++) {
        s.sockets[i] = 0;
    }
    return s;
}

int notFull(SOCKET* list, int size) {
    for(int i=0; i<size; i++) {
        if(list[i] == 0) {
            return i;
        }
    }
    return -1;
}

void acceptLoop(SOCKET serverSocket) {
    server s = newServer(serverSocket, BACKLOG);
    while(1) {
        if(s.exit == 1) {
            for (int i=0; i<BACKLOG; i++) {
                if(s.sockets[i] > 0) {
                    closesocket(s.sockets[i]);
                }
            }
            break;
        }
        s.client = notFull(s.sockets, BACKLOG);
        if(s.client > -1) {
            s.sockets[s.client] = accept(s.serverSocket, NULL, NULL);
            if(s.sockets[s.client] != INVALID_SOCKET) {
                pthread_mutex_lock(&s.lock);
                s.prev = s.client;
                pthread_mutex_unlock(&s.lock);
                pthread_create(&s.th[s.prev], NULL, recvLoop, &s);
                printf("\n[accept() resolved successfully]\n");
            }
        }
    }
}

void* recvLoop(void* pServer) {
    server* s = (server*)pServer;
    pthread_mutex_lock(&s->lock);
    int cur = s->prev;
    pthread_mutex_unlock(&s->lock);
    SOCKET client_socket = s->sockets[cur];
    char buffer[BUFSIZE];
    int recvSize;

    while(1) {
        recvSize = recv(client_socket, buffer, BUFSIZE, 0);
        if(recvSize > 0) {
            buffer[recvSize] = '\0';
            printf("\n[%s]\n", buffer);
            resend(buffer, cur, s);
            if(strstr(buffer, "exit()") != NULL) {
                printf("[recv() exit at socket[%d]]\n\n", client_socket);
                closesocket(client_socket);
                s->sockets[cur] = 0;
                break;
            }
            else if(strstr(buffer, "shutdown()") != NULL) {
                s->exit = 1;
                printf("[recv() shutdown at socket[%d]]\n\n", client_socket);
                for (int i=0; i<BACKLOG; i++) {
                    if(s->sockets[i] > 0) {
                        closesocket(s->sockets[i]);
                    }
                }
                shutdown(s->serverSocket, SD_BOTH);
                closesocket(s->serverSocket);
                WSACleanup();
                exit(0);
            }
        } else {
            printf("[recv() at socket[%d] failed]\n\n", client_socket);
            closesocket(client_socket);
            s->sockets[cur] = 0;
            break;
        }
    }
}

void resend(char* buffer, int sender, server* s) {
    for(int i=0; i < BACKLOG; i++) {
        if(i != sender && s->sockets[i] > 0) {
            if(send(s->sockets[i], buffer, BUFSIZE, 0) == SOCKET_ERROR) {
                printf("Resent to member[%d] socket[%d] failed\n", i, s->sockets[i]);
                closesocket(s->sockets[i]);
                s->sockets[i] = 0;
            } else {
                printf("Resent to member[%d] socket[%d] successfully\n", i, s->sockets[i]);
            }
        }
    }
}

