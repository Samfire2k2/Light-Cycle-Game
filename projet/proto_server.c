#include <stdlib.h>
#include <stdio.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h> // pour fork et execv
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define TAILLE_MAX_NOM 256
#define PORT 5000
#define MAX_PLAYERS 2 // Set to 2 for testing; adjust as needed

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;
typedef struct servent servent;

typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    Position positions[1200];  // Positions des joueur (jusqu'à 1200 segments)
    int length;              // Longueur actuelle de la trace
    int direction;           // Direction actuelle (0: droite, 1: haut, 2: gauche, 3: bas)
    int socket;
} Player;

Player players[MAX_PLAYERS]; // Array to store players
int client_counter = 0; // Global counter for client connections
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for thread-safe counter increment

void init_player(Player* player, int socket) {
    player->length = 2;
    player->direction = 0;
    player->socket = socket;
    for (int i = 0; i < player->length; i++) {
        player->positions[i].x = i;
        player->positions[i].y = 0;
    }
}

typedef struct {
    int sock;
    int player_index;
} ThreadArgs;

void* handle_client(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    int sock = args->sock;
    int player_index = args->player_index;
    free(arg); // Free the allocated memory for the socket descriptor
    Player* player = &players[player_index]; // Get the player for this client

    char buffer[256];
    int longueur;

    while ((longueur = read(sock, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[longueur] = '\0';
        printf("Message from client %d: %s\n", player_index, buffer);

        // Update player direction based on received message
        if (strcmp(buffer, "UP") == 0) player->direction = 1;
        else if (strcmp(buffer, "DOWN") == 0) player->direction = 3;
        else if (strcmp(buffer, "LEFT") == 0) player->direction = 2;
        else if (strcmp(buffer, "RIGHT") == 0) player->direction = 0;
    }

    close(sock); // Close the client socket
    return NULL;
}

void* game_loop(void* arg) {
    while (1) {
        sleep(1); // Wait for 1 second

        pthread_mutex_lock(&counter_mutex);
        for (int i = 0; i < MAX_PLAYERS; i++) {
            Player* player = &players[i];
            char buffer[256];
            int longueur;

            // Request new direction from client
            if (write(player->socket, "DIRECTION", strlen("DIRECTION")) < 0) {
                perror("Error writing to client");
                continue;
            }

            if ((longueur = read(player->socket, buffer, sizeof(buffer) - 1)) > 0) {
                buffer[longueur] = '\0';
                printf("New direction from client %d: %s\n", i, buffer);

                // Update player direction based on received message
                if (strcmp(buffer, "UP") == 0) player->direction = 1;
                else if (strcmp(buffer, "DOWN") == 0) player->direction = 3;
                else if (strcmp(buffer, "LEFT") == 0) player->direction = 2;
                else if (strcmp(buffer, "RIGHT") == 0) player->direction = 0;
            } else {
                perror("Error reading from client");
            }
        }
        pthread_mutex_unlock(&counter_mutex);
    }
    return NULL;
}

int main(int argc, char **argv) {
    int socket_descriptor, nouv_socket_descriptor, longueur_adresse_courante;
    sockaddr_in adresse_locale, adresse_client_courant;
    hostent *ptr_hote;
    servent *ptr_service;
    char machine[TAILLE_MAX_NOM + 1];

    gethostname(machine, TAILLE_MAX_NOM);

    if ((ptr_hote = gethostbyname(machine)) == NULL) {
        perror("erreur : impossible de trouver le serveur a partir de son nom.");
        exit(1);
    }

    bcopy((char *)ptr_hote->h_addr, (char *)&adresse_locale.sin_addr, ptr_hote->h_length);
    adresse_locale.sin_family = ptr_hote->h_addrtype;
    adresse_locale.sin_addr.s_addr = INADDR_ANY;
    adresse_locale.sin_port = htons(PORT);

    printf("numero de port pour la connexion au serveur : %d \n", ntohs(adresse_locale.sin_port));

    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("erreur : impossible de creer la socket de connexion avec le client.");
        exit(1);
    }

    if ((bind(socket_descriptor, (sockaddr *)(&adresse_locale), sizeof(adresse_locale))) < 0) {
        perror("erreur : impossible de lier la socket a l'adresse de connexion.");
        exit(1);
    }

    listen(socket_descriptor, 5);

    while (1) {
        longueur_adresse_courante = sizeof(adresse_client_courant);

        if ((nouv_socket_descriptor = accept(socket_descriptor, (sockaddr *)(&adresse_client_courant), &longueur_adresse_courante)) < 0) {
            perror("erreur : impossible d'accepter la connexion avec le client.");
            exit(1);
        }

        pthread_mutex_lock(&counter_mutex);
        if (client_counter >= MAX_PLAYERS) {
            pthread_mutex_unlock(&counter_mutex);
            printf("Maximum number of players reached. Connection rejected.\n");
            close(nouv_socket_descriptor);
            continue;
        }

        int player_index = client_counter;
        Player* new_player = &players[player_index];
        init_player(new_player, nouv_socket_descriptor);
        client_counter++;
        printf("Client connected. Total clients: %d\n", client_counter);
        pthread_mutex_unlock(&counter_mutex);

        ThreadArgs* args = malloc(sizeof(ThreadArgs));
        args->sock = nouv_socket_descriptor;
        args->player_index = player_index;
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, args);
        pthread_detach(thread);

        if (client_counter == MAX_PLAYERS) {
            // Start the game loop in a separate thread
            pthread_t game_thread;
            pthread_create(&game_thread, NULL, game_loop, NULL);
            pthread_detach(game_thread);
        }
    }

    close(socket_descriptor);
    return 0;
}