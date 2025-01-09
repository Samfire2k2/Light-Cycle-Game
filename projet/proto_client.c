#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5000
#define BUFFER_SIZE 256

int main(int argc, char* argv[]) {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char* directions[] = {"UP", "DOWN", "LEFT", "RIGHT"};
    int direction_index = 0;

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

    // Continuously read messages from the server
    while (1) {
        int length = read(client_socket, buffer, sizeof(buffer) - 1);
        if (length <= 0) {
            perror("Error reading from server");
            break;
        }

        buffer[length] = '\0';
        printf("Message from server: %s\n", buffer);

        // If the server requests a direction, respond
        if (strcmp(buffer, "DIRECTION") == 0) {
            char* direction = directions[direction_index];
            write(client_socket, direction, strlen(direction));
            printf("Sent direction: %s\n", direction);

            // Cycle through directions
            direction_index = (direction_index + 1) % 4;
        }
        else if (strcmp(buffer, "EXIT") == 0) {
            printf("Received EXIT command. Exiting...\n");
            break;
        }
    }

    close(client_socket);
    printf("Disconnected from server\n");
    return 0;
}