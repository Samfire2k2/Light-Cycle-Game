#include <stdlib.h>
#include <stdio.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h> // pour fork et execv
#include <arpa/inet.h>

#define TAILLE_MAX_NOM 256
#define PORT 5000
#define NB_PLAYER 2

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;
typedef struct servent servent;

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

int client_counter = 0; // Global counter for client connections

void renvoi(int sock) {
    char buffer[256];
    int longueur;

    if ((longueur = read(sock, buffer, sizeof(buffer))) <= 0)
        return;

    printf("message lu : %s \n", buffer);

    buffer[0] = 'R';
    buffer[1] = 'E';
    buffer[longueur] = '#';
    buffer[longueur + 1] = '\0';

    printf("message apres traitement : %s \n", buffer);

    printf("renvoi du message traite.\n");

    sleep(3);

    write(sock, buffer, strlen(buffer) + 1);

    printf("message envoye. \n");

    return;
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

     while (client_counter < NB_PLAYER) {
        longueur_adresse_courante = sizeof(adresse_client_courant);

        if ((nouv_socket_descriptor = accept(socket_descriptor, (sockaddr *)(&adresse_client_courant), &longueur_adresse_courante)) < 0) {
            perror("erreur : impossible d'accepter la connexion avec le client.");
            exit(1);
        }

        client_counter++;
        printf("Client connected. Total clients: %d\n", client_counter);

        if (fork() == 0) {
            // close(socket_descriptor);
            renvoi(nouv_socket_descriptor);
            // close(nouv_socket_descriptor);
            exit(0);
        } else {
            // close(nouv_socket_descriptor);
        }
    }

    return 0;
}