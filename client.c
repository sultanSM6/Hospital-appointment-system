#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int read_size;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        printf("Invalid address\n");
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    printf("Connected to the server.\n");

    while ((read_size = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';
        printf("%s", buffer);
        if (strstr(buffer, "Please choose an option:")) {
            break;
        }
    }

    while (1) {
        printf("You: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
            perror("Message sending failed");
            break;
        }

        read_size = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (read_size > 0) {
            buffer[read_size] = '\0';
            printf("Server: %s\n", buffer);
        } else if (read_size == 0) {
            printf("Server disconnected.\n");
            break;
        } else {
            perror("Message receiving failed");
            break;
        }

        if (strcmp(buffer, "4") == 0) {
            printf("Exiting...\n");
            break;
        }
    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}
