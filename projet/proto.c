#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define GRID_SIZE 20
#define GRID_WIDTH (WINDOW_WIDTH / GRID_SIZE)
#define GRID_HEIGHT (WINDOW_HEIGHT / GRID_SIZE)

typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    Position positions[1200];  // Positions du joueur (jusqu'à 100 segments)
    int length;              // Longueur actuelle de la trace
    int direction;           // Direction actuelle (0: droite, 1: haut, 2: gauche, 3: bas)
} Player;

Player player;
int game_over = 0;

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

void init_game() {
    player.length = 2;
    player.direction = 0; // Direction initiale à droite
    for (int i = 0; i < player.length; i++) {
        player.positions[i].x = GRID_WIDTH / 2 - i;
        player.positions[i].y = GRID_HEIGHT / 2;
    }

    srand(time(0));
    // Couleur de fond
    SDL_SetRenderDrawColor(renderer, 54, 67, 95, 255); //RGB pour bleu foncé
    SDL_RenderClear(renderer);
}

void draw_rect(int x, int y, SDL_Color color) {
    SDL_Rect rect = {x * GRID_SIZE, y * GRID_SIZE, GRID_SIZE, GRID_SIZE};
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

void draw_game() {  
    // Dessine le joueur
    SDL_Color player_color = {0, 0, 255, 255}; // bleu
    for (int i = 0; i < player.length; i++) {
        draw_rect(player.positions[i].x, player.positions[i].y, player_color);
    }

    SDL_RenderPresent(renderer); // Met à jour l'affichage
}

void move_player() {
    Position next_position = player.positions[0];

    switch (player.direction) {
        case 0: next_position.x++; break; // Droite
        case 1: next_position.y--; break; // Haut
        case 2: next_position.x--; break; // Gauche
        case 3: next_position.y++; break; // Bas
    }

    player.length++;
    for (int i = player.length - 1; i > 0; i--) {
            player.positions[i] = player.positions[i - 1];
    }

    // Met à jour la position du joueur
    player.positions[0] = next_position;
}

int check_collision() {
    // Collision avec les murs
    if (player.positions[0].x < 0 || player.positions[0].x >= GRID_WIDTH ||
        player.positions[0].y < 0 || player.positions[0].y >= GRID_HEIGHT) {
        return 1;
    }

    // Collision avec soi-même
    for (int i = 1; i < player.length; i++) {
        if (player.positions[0].x == player.positions[i].x &&
            player.positions[0].y == player.positions[i].y) {
            return 1;
        }
    }

    return 0;
}


int main(int argc, char* args[]) {
    // Initialisation de SDL2
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Erreur SDL: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("Snake en SDL2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Erreur SDL: %s\n", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
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
        // Gestion des événements
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_RIGHT: if (player.direction != 2) player.direction = 0; break;
                    case SDLK_UP: if (player.direction != 3) player.direction = 1; break;
                    case SDLK_LEFT: if (player.direction != 0) player.direction = 2; break;
                    case SDLK_DOWN: if (player.direction != 1) player.direction = 3; break;
                }
            }
        }

        Uint32 current_time = SDL_GetTicks();
        if (current_time - last_frame_time >= frame_delay) {
            move_player();
            if (check_collision()) {
                game_over = 1;
            }
            draw_game();
            last_frame_time = current_time;
        }
    }

    printf("Game Over! Votre score: %d\n", player.length - 1);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}