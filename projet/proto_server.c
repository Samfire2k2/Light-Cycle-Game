#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL_image.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define GRID_SIZE 20
#define GRID_WIDTH (WINDOW_WIDTH / GRID_SIZE)
#define GRID_HEIGHT (WINDOW_HEIGHT / GRID_SIZE)
#define PORT 5000

typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    Position positions[1200];
    int length;
    int direction;
} Player;

Player player;
int game_over = 0;

void init_game() {
    player.length = 2;
    player.direction = 0;
    for (int i = 0; i < player.length; i++) {
        player.positions[i].x = GRID_WIDTH / 2 - i;
        player.positions[i].y = GRID_HEIGHT / 2;
    }
    srand(time(0));
}

void move_player() {
    Position next_position = player.positions[0];
    switch (player.direction) {
        case 0: next_position.x++; break;
        case 1: next_position.y--; break;
        case 2: next_position.x--; break;
        case 3: next_position.y++; break;
    }
    player.length++;
    for (int i = player.length - 1; i > 0; i--) {
        player.positions[i] = player.positions[i - 1];
    }
    player.positions[0] = next_position;
}

int check_collision() {
    if (player.positions[0].x < 0 || player.positions[0].x >= GRID_WIDTH ||
        player.positions[0].y < 0 || player.positions[0].y >= GRID_HEIGHT) {
        return 1;
    }
    for (int i = 1; i < player.length; i++) {
        if (player.positions[0].x == player.positions[i].x &&
            player.positions[0].y == player.positions[i].y) {
            return 1;
        }
    }
    return 0;
}

void handle_client(int client_socket) {
    char buffer[256];
    int length;
    while ((length = read(client_socket, buffer, sizeof(buffer))) > 0) {
        buffer[length] = '\0';
        if (strcmp(buffer, "UP") == 0 && player.direction != 3) player.direction = 1;
        else if (strcmp(buffer, "DOWN") == 0 && player.direction != 1) player.direction = 3;
        else if (strcmp(buffer, "LEFT") == 0 && player.direction != 0) player.direction = 2;
        else if (strcmp(buffer, "RIGHT") == 0 && player.direction != 2) player.direction = 0;

        move_player();
        if (check_collision()) {
            game_over = 1;
            strcpy(buffer, "GAME_OVER");
        } else {
            strcpy(buffer, "OK");
        }
        write(client_socket, buffer, strlen(buffer));
    }
    close(client_socket);
}

int main(int argc, char* args[]) {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 3) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    init_game();

    while (!game_over) {
        if ((client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len)) < 0) {
            perror("Accept failed");
            continue;
        }
        handle_client(client_socket);
    }

    close(server_socket);
    return 0;
}