#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <pthread.h>

SOCKET createTCPIPv4Socket() {
    return socket(AF_INET, SOCK_STREAM, 0);
}

SOCKADDR_IN* createIPv4Address(char *ip, int port) {
    SOCKADDR_IN *address = malloc(sizeof(SOCKADDR_IN));
    if (address == NULL)
        return NULL;
    address->sin_family = AF_INET;
    address->sin_port = htons(port);

    if (strlen(ip) == 0)
        address->sin_addr.s_addr = INADDR_ANY;
    else
        address->sin_addr.s_addr = inet_addr(ip);

    return address;
}

void* recvPrint(void* socketFD) {
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

void recvPrintThread(SOCKET *socketFD) {
    pthread_t id;
    pthread_create(&id, NULL, recvPrint, socketFD);
}

int main() {
    WSADATA WSAData;
    WSAStartup(MAKEWORD(2, 0), &WSAData);

    SOCKET socketFD = createTCPIPv4Socket();
    if (socketFD == INVALID_SOCKET) {
        printf("[socket() failed]\n\n");
        WSACleanup();
        system("pause");
        return -1;
    }
    printf("[socket() resolved successfully]\n");

    SOCKADDR_IN *address = createIPv4Address("127.0.0.1", 3000);
    if (address == NULL) {
        printf("[createIPv4Address() failed]\n\n");
        closesocket(socketFD);
        WSACleanup();
        system("pause");
        return -1;
    }
    printf("[createIPv4Address() resolved successfully]\n");

    int connect_result = connect(socketFD, (SOCKADDR *)address, sizeof(*address));
    if (connect_result == SOCKET_ERROR) {
        printf("[Connection failed]\n\n");
        closesocket(socketFD);
        free(address);
        WSACleanup();
        system("pause");
        return -1;
    }
    printf("[Connected to server]\n");

    char client_name[22];
    printf("\nType an alias for this client: ");
    fgets(client_name, sizeof(client_name), stdin);
    client_name[strlen(client_name) - 1] = '\0';

    recvPrintThread(&socketFD);
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
        int result_send = send(socketFD, buffer, msg_len, 0);
        if (result_send == SOCKET_ERROR || strstr(message, "exit()") != NULL)
            break;
    }

    closesocket(socketFD);
    free(address);
    WSACleanup();
    printf("\n");
    system("pause");
    return 0;
}
