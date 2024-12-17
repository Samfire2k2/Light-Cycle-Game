#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5000
#define BUFFER_SIZE 256

void handle_server_response(int sock) {
    char buffer[BUFFER_SIZE];
    int length;

    while ((length = read(sock, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[length] = '\0';
        printf("Message from server: %s\n", buffer);

        if (strcmp(buffer, "EXIT") == 0) {
            printf("Received EXIT command. Exiting...\n");
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    int client_socket;
    struct sockaddr_in server_addr;

    // Create socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Connect to server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server at %s:%d\n", SERVER_IP, SERVER_PORT);

    // Handle server responses
    handle_server_response(client_socket);

    // Close socket
    close(client_socket);
    printf("Disconnected from server\n");

    return 0;
}