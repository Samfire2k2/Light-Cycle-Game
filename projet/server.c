#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT 5000
#define GRID_WIDTH 40
#define GRID_HEIGHT 30

typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    Position positions[1200];
    int length;
    int direction;
    int socket;
    int active;
    int color;
} Player;

int MAX_PLAYERS;
Player* players = NULL;
int client_counter = 0;
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
int game_started = 0;
int game_running = 1;
int server_socket;


void init_player(Player* player, int socket, int index) {
    player->length = 2;
    player->socket = socket;
    player->active = 1;

    // Direction initiale selon la position de départ
    switch(index) {
        case 0: // Haut gauche -> vers la droite
            player->direction = 0;
            player->positions[0].x = 1;
            player->positions[0].y = 1;
            player->color = 1; // Rouge
            break;
        case 1: // Haut droite -> vers le bas
            player->direction = 3;
            player->positions[0].x = GRID_WIDTH - 2;
            player->positions[0].y = 1;
            player->color = 2; // Vert
            break;
        case 2: // Bas gauche -> vers le haut
            player->direction = 1;
            player->positions[0].x = 1;
            player->positions[0].y = GRID_HEIGHT - 2;
            player->color = 3; // Bleu
            break;
        case 3: // Bas droite -> vers la gauche
            player->direction = 2;
            player->positions[0].x = GRID_WIDTH - 2;
            player->positions[0].y = GRID_HEIGHT - 2;
            player->color = 4; // Jaune
            break;
    }

    // Initialiser la traînée
    for (int i = 1; i < player->length; i++) {
        player->positions[i] = player->positions[0];
    }
}

int check_collision(Player* player) {
    // Vérification des collisions avec les murs
    if (player->positions[0].x < 0 || player->positions[0].x >= GRID_WIDTH ||
        player->positions[0].y < 0 || player->positions[0].y >= GRID_HEIGHT) {
        printf("Wall collision detected for player at position (%d,%d)\n", 
               player->positions[0].x, player->positions[0].y);
        return 1;
    }

    // Vérification des collisions avec sa propre traînée
    for (int i = 1; i < player->length; i++) {
        if (player->positions[0].x == player->positions[i].x &&
            player->positions[0].y == player->positions[i].y) {
            printf("Self collision detected for player at position (%d,%d)\n", 
                   player->positions[0].x, player->positions[0].y);
            return 1;
        }
    }

    // Vérification des collisions avec les autres joueurs et leurs traînées
    for (int p = 0; p < MAX_PLAYERS; p++) {
        if (!players[p].active || players[p].socket == player->socket) 
            continue;
        
        // Vérifie la collision avec toute la traînée de l'autre joueur
        for (int i = 0; i < players[p].length; i++) {
            if (player->positions[0].x == players[p].positions[i].x &&
                player->positions[0].y == players[p].positions[i].y) {
                printf("Collision detected between players at position (%d,%d)\n", 
                       player->positions[0].x, player->positions[0].y);
                return 1;
            }
        }
    }
    
    return 0;
}

void* handle_client(void* arg) {
    int player_index = *(int*)arg;
    free(arg);  // Libérer la mémoire allouée
    char buffer[256];
    while (game_running) {
        int len = read(players[player_index].socket, buffer, sizeof(buffer) - 1);
        if (len > 0) {
            buffer[len] = '\0';
            if (strcmp(buffer, "UP\n") == 0 && players[player_index].direction != 3) players[player_index].direction = 1;
            if (strcmp(buffer, "DOWN\n") == 0 && players[player_index].direction != 1) players[player_index].direction = 3;
            if (strcmp(buffer, "LEFT\n") == 0 && players[player_index].direction != 0) players[player_index].direction = 2;
            if (strcmp(buffer, "RIGHT\n") == 0 && players[player_index].direction != 2) players[player_index].direction = 0;
        }
    }
    return NULL;
}

void* game_loop(void* arg) {
    while (client_counter < MAX_PLAYERS) {
        sleep(1);
    }

    printf("Starting countdown...\n");
    for (int i = 3; i > 0; i--) {
        for (int p = 0; p < MAX_PLAYERS; p++) {
            if (players[p].active) {
                char countdown = '0' + i;
                write(players[p].socket, &countdown, 1);
            }
        }
        sleep(1);
    }

// Envoyer le signal de début
for (int p = 0; p < MAX_PLAYERS; p++) {
    if (players[p].active) {
        write(players[p].socket, "S", 1);
    }
}

    game_started = 1;
    printf("Game has started!\n");
    
    while (1) {
        usleep(100000);
        pthread_mutex_lock(&counter_mutex);
        int active_players = 0;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (!players[i].active) continue;
            active_players++;
            
            for (int j = players[i].length - 1; j > 0; j--) {
                players[i].positions[j] = players[i].positions[j - 1];
            }
            
            switch (players[i].direction) {
                case 0: players[i].positions[0].x++; break;
                case 1: players[i].positions[0].y--; break;
                case 2: players[i].positions[0].x--; break;
                case 3: players[i].positions[0].y++; break;
            }
            // Augmente la longueur du joueur jusqu'à la limite
            if (players[i].length < 1200) {
                players[i].length++;
            }


            if (check_collision(&players[i])) {
                players[i].active = 0;
                printf("Player %d has lost!\n", i);
            }
        }


        if (active_players <= 1) {
            int winner = -1;
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (players[i].active) {
                    winner = i;
                    break;
                }
            }
            
            printf("Game over! Player %d won!\n", winner);
            char game_over_msg[32];  
            snprintf(game_over_msg, sizeof(game_over_msg), "GAME_OVER %d", winner);
            
            for (int i = 0; i < MAX_PLAYERS; i++) {
                write(players[i].socket, game_over_msg, strlen(game_over_msg));
                close(players[i].socket);
            }
            game_running = 0;
            pthread_mutex_unlock(&counter_mutex);
            return NULL;
        }
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (players[i].active) {
                char buffer[4096];
                int offset = 0;
                for (int p = 0; p < MAX_PLAYERS; p++) {
                    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%d %d %d ",
                                       players[p].positions[0].x, players[p].positions[0].y, players[p].color);
                }
                buffer[offset - 1] = '\0';

                write(players[i].socket, buffer, strlen(buffer));
            }
        }
        pthread_mutex_unlock(&counter_mutex);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <nb_players>\n", argv[0]);
        printf("Example: %s 2\n", argv[0]);
        return 1;
    }

    MAX_PLAYERS = atoi(argv[1]);
    if (MAX_PLAYERS < 2) {
        printf("Error: Number of players must be greater yhan 2\n");
        return 1;
    }

    players = (Player*)malloc(MAX_PLAYERS * sizeof(Player));
    if (players == NULL) {
        printf("Error: Memory allocation failed\n");
        return 1;
    }

    int client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_socket, MAX_PLAYERS);

    printf("Server started. Waiting for players...\n");

    pthread_t game_thread;
    pthread_create(&game_thread, NULL, game_loop, NULL);
    pthread_detach(game_thread);

    while (client_counter < MAX_PLAYERS) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        write(client_socket, &MAX_PLAYERS, sizeof(int));
        pthread_mutex_lock(&counter_mutex);
        int player_index = client_counter;
        init_player(&players[player_index], client_socket, player_index);
        pthread_t client_thread;
        // Allouer un nouvel entier sur le tas pour stocker l'index
        int* thread_arg = malloc(sizeof(int));
        *thread_arg = player_index;
        pthread_create(&client_thread, NULL, handle_client, thread_arg);
        pthread_detach(client_thread);
        client_counter++;
        pthread_mutex_unlock(&counter_mutex);
    }
    
    printf("All players connected. Game starting...\n");
    
    while (game_running) {
        sleep(1);
    }
    
    // Nettoyage
    close(server_socket);
    free(players);
    printf("Server shutting down...\n");
    return 0;
}
