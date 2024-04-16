#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <pthread.h>

// IPv4 address
SOCKADDR_IN newAddress(char *ip, int port);

void* sendLoop(void* socket);

void* recvLoop(void* socket);

int main() {
    WSADATA WSAData;
    SOCKET client_socket;
    SOCKADDR_IN server_addr;
    pthread_t id;

    if (WSAStartup(MAKEWORD(2, 0), &WSAData) != 0) {
        printf("[WSAStartup() failed]\n\n");
        system("pause");
        exit(1);
    }
    printf("[WSAStartup() resolved successfully]\n");

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        printf("[socket() failed]\n\n");
        WSACleanup();
        system("pause");
        exit(1);
    }
    printf("[socket() resolved successfully]\n");

    server_addr = newAddress("127.0.0.1", 3000);

    if (connect(client_socket, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("[connect() failed]\n\n");
        closesocket(client_socket);
        WSACleanup();
        system("pause");
        exit(1);
    }
    printf("[connect() resolved successfully]\n");

    pthread_create(&id, NULL, recvLoop, &client_socket);

    sendLoop(&client_socket);

    closesocket(client_socket);
    WSACleanup();
    printf("\n");
    system("pause");
    return 0;
}

SOCKADDR_IN newAddress(char *ip, int port) {
    SOCKADDR_IN addr_in;
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(port);
    addr_in.sin_addr.s_addr = (ip == NULL || strlen(ip) == 0) ? INADDR_ANY : inet_addr(ip);
    return addr_in;
}

void* sendLoop(void* socket) {
    SOCKET client_socket = *(SOCKET*)socket;
    char username[22], message[1000], buffer[1024];
    int msg_len;
    
    printf("\nType an alias for this client: ");
    fgets(username, sizeof(username), stdin);
    username[strlen(username) - 1] = '\0';
    while (1) {
        //printf("\nType a message to send:\n\n");
        fgets(message, sizeof(message), stdin);
        msg_len = strlen(message);

        if (msg_len > 1) {
            message[msg_len - 1] = '\0';
            snprintf(buffer, sizeof(buffer), "%s: %s", username, message);
            msg_len = strlen(buffer);
            if (send(client_socket, buffer, msg_len, 0) == SOCKET_ERROR || strstr(message, "exit()") != NULL || strstr(message, "shutdown()") != NULL)
                break;
        }
    }
    return NULL;
}

void* recvLoop(void* socket) {
    SOCKET s = *(SOCKET*)socket;
    char buffer[1024];
    int recvSize;
    while(1) {
        recvSize = recv(s, buffer, sizeof(buffer), 0);
        if(recvSize > 0) {
            buffer[recvSize] = '\0';
            printf("[%s]\n\n", buffer);
            if(strstr(buffer, "shutdown()") != NULL) {
                closesocket(s);
                WSACleanup();
                exit(0);
            }
        }
        else {
            printf("[recv() failed]\n\n");
            break;
        }
    }
    return NULL;
}