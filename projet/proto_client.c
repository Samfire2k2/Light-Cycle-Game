#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
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

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture* player_texture = NULL;

void init_game() {
    printf("Initializing game...\n");
    player.length = 2;
    player.direction = 0;
    for (int i = 0; i < player.length; i++) {
        player.positions[i].x = GRID_WIDTH / 2 - i;
        player.positions[i].y = GRID_HEIGHT / 2;
    }
    srand(time(0));
    SDL_SetRenderDrawColor(renderer, 54, 67, 95, 255);
    SDL_RenderClear(renderer);
    player_texture = IMG_LoadTexture(renderer, "../projet/ressources/motoJ1.png");
    if (!player_texture) {
        printf("Erreur de chargement de la texture: %s\n", IMG_GetError());
        exit(1);
    }
    printf("Game initialized.\n");
}

void draw_rect(int x, int y, SDL_Color color) {
    SDL_Rect rect = {x * GRID_SIZE, y * GRID_SIZE, GRID_SIZE, GRID_SIZE};
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

void draw_texture(SDL_Texture* texture, int x, int y, float angle) {
    if (!texture) {
        printf("Erreur: texture est NULL\n");
        return;
    }
    SDL_Rect dst;
    dst.x = x * GRID_SIZE - (GRID_SIZE / 2);
    dst.y = y * GRID_SIZE - (GRID_SIZE / 2);
    dst.w = GRID_SIZE * 2;
    dst.h = GRID_SIZE * 2;
    SDL_RenderCopyEx(renderer, texture, NULL, &dst, angle, NULL, SDL_FLIP_NONE);
}

void draw_game() {
    SDL_SetRenderDrawColor(renderer, 54, 67, 95, 255);
    SDL_RenderClear(renderer);
    for (int i = 2; i < player.length; i++) {
        SDL_Color player_color = {255, 0, 0, 255};
        draw_rect(player.positions[i - 1].x, player.positions[i - 1].y, player_color);
    }
    draw_texture(player_texture, player.positions[0].x, player.positions[0].y, (player.direction + 1) * 90);
    SDL_RenderPresent(renderer);
}

void send_input(int socket, const char* input) {
    write(socket, input, strlen(input));
    char buffer[256];
    int length = read(socket, buffer, sizeof(buffer) - 1);
    if (length > 0) {
        buffer[length] = '\0';
        if (strcmp(buffer, "GAME_OVER") == 0) {
            game_over = 1;
        }
    }
}

int main(int argc, char* args[]) {
    int client_socket;
    struct sockaddr_in server_addr;

    printf("Creating socket...\n");
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Connecting to server...\n");
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Initializing SDL...\n");
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Erreur SDL: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1) {
        printf("Erreur TTF: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    if (IMG_Init(IMG_INIT_PNG) == 0) {
        printf("Erreur IMG_Init: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    printf("Creating window...\n");
    window = SDL_CreateWindow("Tron", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Erreur SDL: %s\n", SDL_GetError());
        return 1;
    }

    printf("Creating renderer...\n");
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        printf("Erreur SDL: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    init_game();

    int quit = 0;
    SDL_Event e;
    Uint32 frame_delay = 60;
    Uint32 last_frame_time = 0;

    while (!quit && !game_over) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_RIGHT: send_input(client_socket, "RIGHT"); break;
                    case SDLK_UP: send_input(client_socket, "UP"); break;
                    case SDLK_LEFT: send_input(client_socket, "LEFT"); break;
                    case SDLK_DOWN: send_input(client_socket, "DOWN"); break;
                }
            }
        }

        Uint32 current_time = SDL_GetTicks();
        if (current_time - last_frame_time >= frame_delay) {
            draw_game();
            last_frame_time = current_time;
        }
    }

    printf("Game Over!\n");

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyTexture(player_texture);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    close(client_socket);
    return 0;
}