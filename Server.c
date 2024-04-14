#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <pthread.h>

#define SERVER_BACKLOG 10
#define BUFFER_SIZE 1024

typedef struct ClientAddrFD {
    SOCKET socketFD;
} clientAddrFD;

typedef struct ClientGroup {
    clientAddrFD* members[SERVER_BACKLOG];
    clientAddrFD* client;
    int size;
    int exit;
} clientGroup;

clientAddrFD* clientAccept(SOCKET serverSocketFD) {
    clientAddrFD* client = malloc(sizeof(clientAddrFD));
    if(client == NULL)
        return NULL;

    client->socketFD = accept(serverSocketFD, NULL, NULL);
    return client;
}

SOCKADDR_IN newAddress(char* ip, int port) {
    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = (ip == NULL || strlen(ip) == 0) ? INADDR_ANY : inet_addr(ip) ;
    return addr;
}


int resend(char *buffer, SOCKET sender_socket, clientGroup *group) {
    for(int i=0; i<group->size; i++) {
        if(group->members[i]->socketFD > 0 && sender_socket != group->members[i]->socketFD) {
            if(send(group->members[i]->socketFD, buffer, BUFFER_SIZE, 0) == SOCKET_ERROR) {
                closesocket(group->members[i]->socketFD);
                if(group->client->socketFD == group->members[i]->socketFD)
                    group->client->socketFD = 0;
                group->members[i]->socketFD = 0;
            }
            else
                printf("Resent to member[%d] socket: %d\n", i, group->members[i]->socketFD);
        }
    }
    return 0;
}


void* recvPrint(void* clGroup) {
    clientGroup* group = (clientGroup*)clGroup;
    char buffer[BUFFER_SIZE];
    SOCKET socketFD = group->client->socketFD;
    while(1) {
        int recvSize = recv(socketFD, buffer, BUFFER_SIZE, 0);
        if(recvSize > 0) {
            buffer[recvSize] = '\0';
            printf("\n[%s]\n", buffer);
            resend(buffer, socketFD, group);
            if(strstr(buffer, "exit()") != NULL) {
                closesocket(socketFD);
                for(int i=0; i<group->size; i++) {
                    if(socketFD == group->members[i]->socketFD)
                        group->members[i]->socketFD = 0;
                }
                if(socketFD == group->client->socketFD)
                    group->client->socketFD = 0;
                break;
            }
            else if(strstr(buffer, "shutdown()") != NULL) {
                group->exit = 1;
                break;
            }
        }
        else {
            break;
        }
    }
}


void recvPrintThread(clientGroup *group) {
    pthread_t id;
    pthread_create(&id, NULL, recvPrint, group);
}


int main() {
    WSADATA WSAData;
    SOCKET serverSocketFD;
    SOCKADDR_IN serverAddress;

    WSAStartup(MAKEWORD(2,0), &WSAData);

    serverSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocketFD == INVALID_SOCKET) {
        printf("[socket() failed]\n\n");
        WSACleanup();
        system("pause");
        exit(1);
    }
    printf("[socket() resolved successfully]\n");

    serverAddress = newAddress("", 3000);

    if(bind(serverSocketFD, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) != 0) {
        printf("[bind() failed]\n\n");
        closesocket(serverSocketFD);
        WSACleanup();
        system("pause");
        exit(1);
    }
    printf("[bind() resolved successfully]\n");


    if(listen(serverSocketFD, SERVER_BACKLOG) != 0){
        printf("[listen() failed]\n\n");
        closesocket(serverSocketFD);
        WSACleanup();
        system("pause");
        exit(1);
    }
    printf("[listen() resolved successfully]\n");

    clientGroup group;
    group.size = 0;
    group.exit = 0;
    while(1){
        if(group.exit == 1)
            break;
        if(group.size < SERVER_BACKLOG){
            group.members[group.size] = clientAccept(serverSocketFD);

            group.client = group.members[group.size];
            if(group.client != NULL && group.client->socketFD != INVALID_SOCKET) {
                group.size++;
                printf("\n[accept() resolved successfully]\n");
                recvPrintThread(&group);
            }
        }
    }


    shutdown(serverSocketFD, SD_BOTH);
    closesocket(serverSocketFD);
    WSACleanup();
    printf("\n\n");
    system("pause");
    return 0;
}
