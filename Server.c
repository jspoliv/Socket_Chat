#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <pthread.h>

typedef struct ClientAddrFD {
    SOCKADDR addr;
    int addrSize;
    SOCKET socketFD;
} clientAddrFD;

typedef struct ClientGroup {
    clientAddrFD* members[10];
    clientAddrFD* client;
    int size;
    int exit;
} clientGroup;

clientAddrFD* clientAccept(SOCKET serverSocketFD) {
    clientAddrFD* client = malloc(sizeof(clientAddrFD));
    if(client == NULL)
        return NULL;

    client->addrSize = sizeof(client->addr);
    client->socketFD = accept(serverSocketFD, &client->addr, &client->addrSize);
    //printf("clientFD: %d \nserverFD: %d\n", client->socketFD, serverSocketFD);
    return client;
}

SOCKET createTCPIPv4Socket() {
    return socket(AF_INET, SOCK_STREAM, 0);
}

SOCKADDR_IN* createIPv4Address(char* ip, int port) {
    SOCKADDR_IN* address = malloc(sizeof(SOCKADDR_IN));
    if(address == NULL)
        return NULL;
    address->sin_family = AF_INET;
    address->sin_port = htons(port);

    if(strlen(ip) == 0)
        address->sin_addr.s_addr = INADDR_ANY;
    else
        address->sin_addr.s_addr = inet_addr(ip);

    return address;
}


int resend(char *buffer, SOCKET socketFD, clientGroup *group) {
    for(int i=0; i<group->size; i++) {
        if(group->members[i]->socketFD > 0 && socketFD != group->members[i]->socketFD) {
            int sendResult = send(group->members[i]->socketFD, buffer, 1024, 0);
            if(sendResult == SOCKET_ERROR) {
                closesocket(group->members[i]->socketFD);
                if(group->client->socketFD == group->members[i]->socketFD)
                    group->client->socketFD = 0;
                group->members[i]->socketFD = 0;
            }
            else
                printf("\nResend: (%d) %d\n", i, group->members[i]->socketFD);
        }
    }
    return 0;
}


void* recvPrint(void* clGroup) {
    clientGroup* group = (clientGroup*)clGroup;
    char buffer[1024];
    SOCKET socketFD = group->client->socketFD;
    while(1) {
        int recvSize = recv(socketFD, buffer, 1024, 0);
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
    WSAStartup(MAKEWORD(2,0), &WSAData);

    SOCKET serverSocketFD = createTCPIPv4Socket();
    if(serverSocketFD == INVALID_SOCKET) {
        printf("[socket() failed]\n\n");
        WSACleanup();
        system("pause");
        return -1;
    }
    printf("[socket() resolved successfully]\n");


    SOCKADDR_IN* serverAddress = createIPv4Address("", 3000);
    if(serverAddress == NULL) {
        printf("[createIPv4Address() failed]\n\n");
        closesocket(serverSocketFD);
        WSACleanup();
        system("pause");
        return -1;
    }
    printf("[createIPv4Address() resolved successfully]\n");


    int bindResult = bind(serverSocketFD, (SOCKADDR*)serverAddress, sizeof(*serverAddress));
    if(bindResult != 0) {
        printf("[bind() failed]\n\n");
        closesocket(serverSocketFD);
        free(serverAddress);
        WSACleanup();
        system("pause");
        return -1;
    }
    printf("[bind() resolved successfully]\n");


    int listenResult = listen(serverSocketFD, 10);
    if(listenResult != 0){
        printf("[listen() failed]\n\n");
        closesocket(serverSocketFD);
        free(serverAddress);
        WSACleanup();
        system("pause");
        return -1;
    }
    printf("[listen() resolved successfully]\n");

    clientGroup group;
    group.size = 0;
    group.exit = 0;
    while(1){
        if(group.exit == 1)
            break;
        if(group.size < 10){
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
    free(serverAddress);
    WSACleanup();
    printf("\n\n");
    system("pause");
    return 0;
}
