#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL_image.h>

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

SDL_Texture* load_texture(const char* file, SDL_Renderer* ren) {
    SDL_Texture* texture = IMG_LoadTexture(ren, file);
    if (!texture) {
        printf("Erreur de chargement de la texture: %s\n", SDL_GetError());
    }
    return texture;
}

Player player;
int game_over = 0;

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture* player_texture = NULL;

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
    player_texture = load_texture("../projet/ressources/moto.png", renderer);
}

void draw_rect(int x, int y, SDL_Color color) {
    SDL_Rect rect = {x * GRID_SIZE, y * GRID_SIZE, GRID_SIZE, GRID_SIZE};
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

void draw_texture(SDL_Texture* texture, int x, int y, float angle) {
    SDL_Rect dst;
    dst.x = x * GRID_SIZE - (GRID_SIZE / 2);
    dst.y = y * GRID_SIZE - (GRID_SIZE / 2);
    dst.w = GRID_SIZE * 2;
    dst.h = GRID_SIZE * 2;
    SDL_RenderCopyEx(renderer, player_texture, NULL, &dst, angle, NULL, SDL_FLIP_NONE);  // Dessine la texture
}

// Nouvelle fonction pour dessiner la grille
void draw_grid() {
    SDL_SetRenderDrawColor(renderer, 100, 100, 150, 255);  // Couleur gris clair pour la grille

    // Dessiner les lignes verticales
    for (int x = 0; x <= WINDOW_WIDTH; x += GRID_SIZE) {
        SDL_RenderDrawLine(renderer, x, 0, x, WINDOW_HEIGHT);
    }

    // Dessiner les lignes horizontales
    for (int y = 0; y <= WINDOW_HEIGHT; y += GRID_SIZE) {
        SDL_RenderDrawLine(renderer, 0, y, WINDOW_WIDTH, y);
    }
}

void draw_game() {
    // Dessine le fond
    SDL_SetRenderDrawColor(renderer, 54, 67, 95, 255); // Bleu foncé pour le fond
    SDL_RenderClear(renderer);

    // Dessine la grille par-dessus le fond
    draw_grid();

    // Dessine la trace
    for (int i = 2; i < player.length; i++) {
        SDL_Color player_color = {255, 0, 0, 255}; // Rouge
        draw_rect(player.positions[i - 1].x, player.positions[i - 1].y, player_color);
    }

    // Dessine le joueur
    draw_texture(player_texture, player.positions[0].x, player.positions[0].y, (player.direction + 1) * 90);

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

    window = SDL_CreateWindow("Tron", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Erreur SDL: %s\n", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE );
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
                SDL_Color red_color = {255, 0, 0, 255}; // Couleur rouge pour signaler une collision
                draw_rect(player.positions[0].x, player.positions[0].y, red_color);
                SDL_RenderPresent(renderer);
                SDL_Delay(3000);
                game_over = 1;
            }
            draw_game();
            last_frame_time = current_time;
        }
    }

    printf("Game Over! Votre score: %d\n", player.length - 1);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyTexture(player_texture);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
