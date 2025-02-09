#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <SDL2/SDL.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5000
#define GRID_SIZE 20
#define MAX_LENGTH 1200
#define MAX_PLAYERS 4
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
int running = 1;

// Définition des couleurs pour chaque joueur
const SDL_Color PLAYER_COLORS[MAX_PLAYERS] = {
    {255, 0, 0, 255},    // Rouge
    {0, 255, 0, 255},    // Vert
    {0, 0, 255, 255},    // Bleu
    {255, 255, 0, 255}   // Jaune
};

void draw_rect(int x, int y, SDL_Color color) {
    SDL_Rect rect = {x * GRID_SIZE, y * GRID_SIZE, GRID_SIZE, GRID_SIZE};
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[4096];

    // Initialisation du socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    printf("Connected to server! Waiting for the game to start...\n");

    // Initialisation SDL
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Tron Multiplayer", 
                            SDL_WINDOWPOS_CENTERED, 
                            SDL_WINDOWPOS_CENTERED,
                            WINDOW_WIDTH, 
                            WINDOW_HEIGHT, 
                            SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

    // Initialisation des données des joueurs
    int player_positions[MAX_PLAYERS][MAX_LENGTH][2];
    int player_colors[MAX_PLAYERS];
    int player_lengths[MAX_PLAYERS];

    // Initialisation des tableaux
    for (int i = 0; i < MAX_PLAYERS; i++) {
        memset(player_positions[i], -1, sizeof(player_positions[i]));
        player_lengths[i] = 1;
        player_colors[i] = 0;
    }

    SDL_Event event;
    while (running) {
        // Gestion des événements
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN) {
                char direction[10] = {0};
                if (event.key.keysym.sym == SDLK_UP) strcpy(direction, "UP\n");
                if (event.key.keysym.sym == SDLK_DOWN) strcpy(direction, "DOWN\n");
                if (event.key.keysym.sym == SDLK_LEFT) strcpy(direction, "LEFT\n");
                if (event.key.keysym.sym == SDLK_RIGHT) strcpy(direction, "RIGHT\n");
                if (strlen(direction) > 0) {
                    write(client_socket, direction, strlen(direction));
                }
            }
        }

        // Lecture des données du serveur
        int len = read(client_socket, buffer, sizeof(buffer) - 1);
        if (len > 0) {
            buffer[len] = '\0';
            if (strcmp(buffer, "GAME_OVER") == 0) {
                printf("Game over! Closing client.\n");
                running = 0;
                break;
            }

            // Parsing des données
            char* token = strtok(buffer, " ");
            int player = 0;
            while (token != NULL && player < MAX_PLAYERS) {
                int x = atoi(token);
                token = strtok(NULL, " ");
                int y = atoi(token);
                token = strtok(NULL, " ");
                int color = atoi(token);
                token = strtok(NULL, " ");

                if (x >= 0 && y >= 0) {
                    player_positions[player][player_lengths[player]][0] = x;
                    player_positions[player][player_lengths[player]][1] = y;
                    player_colors[player] = color;
                    if (player_lengths[player] < MAX_LENGTH - 1) {
                        player_lengths[player]++;
                    }
                }
                player++;
            }
        }

        // Rendu
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Dessin des joueurs
        for (int p = 0; p < MAX_PLAYERS; p++) {
            if (player_colors[p] > 0) {
                for (int i = 0; i < player_lengths[p]; i++) {
                    if (player_positions[p][i][0] >= 0 && player_positions[p][i][1] >= 0) {
                        SDL_Color color = PLAYER_COLORS[player_colors[p] - 1];
                        draw_rect(player_positions[p][i][0], player_positions[p][i][1], color);
                    }
                }
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 FPS
    }

    // Nettoyage
    close(client_socket);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}