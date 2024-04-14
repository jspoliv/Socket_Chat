#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <pthread.h>

// IPv4 TCP socket
SOCKET newSocket();

// IPv4 address
SOCKADDR_IN newAddress(char *ip, int port);

void* sendLoop(SOCKET* socket);

void* recvLoop(void* socketFD) {
    SOCKET* socket = (SOCKET*)socketFD;
    char buffer[1024];
    while(1) {
        ssize_t recvSize = recv(*socket, buffer, sizeof(buffer), 0);
        if(recvSize > 0) {
            buffer[recvSize] = '\0';
            printf("[%s]\n\n", buffer);
            //if(strstr(buffer, "exit()") != NULL)
            //    break;
        }
        else {
            break;
        }
    }
}

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

    client_socket = newSocket();
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

SOCKET newSocket() {
    return socket(AF_INET, SOCK_STREAM, 0);
}

SOCKADDR_IN newAddress(char *ip, int port) {
    SOCKADDR_IN addr_in;
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(port);
    addr_in.sin_addr.s_addr = (ip == NULL || strlen(ip) == 0) ? INADDR_ANY : inet_addr(ip);
    return addr_in;
}


void* sendLoop(SOCKET* socket) {
    SOCKET client_socket = *socket;
    char client_name[22];
    printf("\nType an alias for this client: ");
    fgets(client_name, sizeof(client_name), stdin);
    client_name[strlen(client_name) - 1] = '\0';
    char buffer[1024];
    char message[1000];
    while (1) {
        //printf("\nType a message to send:\n\n");
        fgets(message, sizeof(message), stdin);
        int msg_len = strlen(message);

        if (msg_len == 1)
            continue;
        else
        {
            message[msg_len - 1] = '\0';
            snprintf(buffer, sizeof(buffer), "%s: %s", client_name, message);
            msg_len = strlen(buffer);
        }
        int result_send = send(client_socket, buffer, msg_len, 0);
        if (result_send == SOCKET_ERROR || strstr(message, "exit()") != NULL)
            break;
    }
    return NULL;
}

