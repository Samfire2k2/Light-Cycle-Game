#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>


#define DEFAULT_SERVER_IP "127.0.0.1"
#define SERVER_PORT 5000
#define GRID_SIZE 20
#define MAX_LENGTH 1200
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
TTF_Font* font = NULL;
int running = 1;
int MAX_PLAYERS;

void draw_rect(int x, int y, SDL_Color color) {
    SDL_Rect rect = {x * GRID_SIZE, y * GRID_SIZE, GRID_SIZE, GRID_SIZE};
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

void draw_grid() {
    // Définir la couleur de la grille (gris foncé)
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    
    // Lignes verticales
    for (int x = 0; x <= WINDOW_WIDTH; x += GRID_SIZE) {
        SDL_RenderDrawLine(renderer, x, 0, x, WINDOW_HEIGHT);
    }
    
    // Lignes horizontales
    for (int y = 0; y <= WINDOW_HEIGHT; y += GRID_SIZE) {
        SDL_RenderDrawLine(renderer, 0, y, WINDOW_WIDTH, y);
    }
}

void draw_countdown(int number) {
    char text[2];
    sprintf(text, "%d", number);
    
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, textColor);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    SDL_Rect dest;
    dest.w = surface->w;
    dest.h = surface->h;
    dest.x = (WINDOW_WIDTH - dest.w) / 2;
    dest.y = (WINDOW_HEIGHT - dest.h) / 2;
    
    SDL_RenderCopy(renderer, texture, NULL, &dest);
    
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void draw_text(const char* text) {
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, textColor);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    SDL_Rect dest;
    dest.w = surface->w;
    dest.h = surface->h;
    dest.x = (WINDOW_WIDTH - dest.w) / 2;
    dest.y = (WINDOW_HEIGHT - dest.h) / 2;
    
    SDL_RenderCopy(renderer, texture, NULL, &dest);
    
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

int main(int argc, char *argv[]) {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[4096];
    const char* server_ip = DEFAULT_SERVER_IP;
    if (argc > 1) {
        server_ip = argv[1];
    }

    // Initialisation du socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    // Recevoir le nombre de joueurs du serveur
    if (read(client_socket, &MAX_PLAYERS, sizeof(int)) < 0) {
        perror("Failed to receive player count");
        exit(EXIT_FAILURE);
    }
    printf("Connected to server! Waiting for the game to start...\n");

    int (*player_positions)[MAX_LENGTH][2] = malloc(MAX_PLAYERS * sizeof(*player_positions));
    int* player_colors = malloc(MAX_PLAYERS * sizeof(int));
    int* player_lengths = malloc(MAX_PLAYERS * sizeof(int));
    SDL_Color* PLAYER_COLORS = malloc(MAX_PLAYERS * sizeof(SDL_Color));

    if (!player_positions || !player_colors || !player_lengths || !PLAYER_COLORS) {
        printf("Error: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Initialiser les couleurs avec un dégradé
    for (int i = 0; i < MAX_PLAYERS; i++) {
        // Dégradé de couleurs
        PLAYER_COLORS[i] = (SDL_Color){
            255 * (i % 3 == 0),     // R
            255 * (i % 3 == 1),     // G
            255 * (i % 3 == 2),     // B
            255                      // A
        };
    }

    // Initialisation SDL
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();  // Initialiser SDL_ttf
    font = TTF_OpenFont("projet/ressources/Consolas.ttf", 32);  // Remplacer par le chemin de votre police
    if (!font) {
        printf("TTF_OpenFont error: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }
    window = SDL_CreateWindow("Tron Multiplayer", 
                            SDL_WINDOWPOS_CENTERED, 
                            SDL_WINDOWPOS_CENTERED,
                            WINDOW_WIDTH, 
                            WINDOW_HEIGHT, 
                            SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

    char countdown_buffer[2];
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    draw_text("En attente de joueurs...");
    SDL_RenderPresent(renderer);

    if (font) {
        TTF_CloseFont(font);
    }
    font = TTF_OpenFont("projet/ressources/Consolas.ttf", 128);

    while (1) {
        int len = read(client_socket, countdown_buffer, 1);
        if (len > 0) {
            if (countdown_buffer[0] == 'S') {
                // Afficher "GO!" avant de commencer
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
                draw_grid();
                draw_text("GO!");
                SDL_RenderPresent(renderer);
                break;
            }
            if (countdown_buffer[0] >= '1' && countdown_buffer[0] <= '3') {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
                draw_grid();
                draw_countdown(countdown_buffer[0] - '0');
                SDL_RenderPresent(renderer);
            }
        }
    }

    if (font) {
        TTF_CloseFont(font);
    }
    font = TTF_OpenFont("projet/ressources/Consolas.ttf", 32);

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
            if (strncmp(buffer, "GAME_OVER", 9) == 0) {
                int winner;
                sscanf(buffer, "GAME_OVER %d", &winner);
                printf("Game over! Player %d won!\n", winner);

                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
                
                // Afficher le message de victoire
                char win_message[50];
                sprintf(win_message, "Joueur %d a gagne !", winner + 1);
                draw_text(win_message);
                SDL_RenderPresent(renderer);
                
                SDL_Delay(3000);  // Attendre 3 secondes avant de fermer
                running = 0;
                continue;
            }else if (running){

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
        }

        // Rendu
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        draw_grid();

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
    if (font) {
        TTF_CloseFont(font);
    }
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    close(client_socket);
    free(player_positions);
    free(player_colors);
    free(player_lengths);
    free(PLAYER_COLORS);

    return 0;
}