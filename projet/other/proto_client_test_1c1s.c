#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#define TAILLE_MAX_NOM 256
#define BUFFER_SIZE 256
#define DEFAULT_PORT 5000

volatile sig_atomic_t running = 1;

void handle_sigint(int sig) {
    running = 0;
}

int main(int argc, char **argv) {
    int socket_descriptor;
    struct sockaddr_in adresse_serveur;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];
    const char* server_ip = (argc > 1) ? argv[1] : "127.0.0.1";

    signal(SIGINT, handle_sigint);
    
    // Socket creation
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erreur création socket");
        exit(1);
    }

    // Server address setup
    memset(&adresse_serveur, 0, sizeof(adresse_serveur));
    adresse_serveur.sin_family = AF_INET;
    adresse_serveur.sin_port = htons(DEFAULT_PORT);
    
    if (inet_pton(AF_INET, server_ip, &adresse_serveur.sin_addr) <= 0) {
        perror("Adresse IP invalide");
        close(socket_descriptor);
        exit(1);
    }

    printf("Tentative de connexion au serveur %s:%d...\n", server_ip, DEFAULT_PORT);

    // Connection to server
    if (connect(socket_descriptor, (struct sockaddr *)&adresse_serveur, 
        sizeof(adresse_serveur)) < 0) {
        perror("Erreur connexion");
        close(socket_descriptor);
        exit(1);
    }

    printf("Connecté au serveur!\n");
    printf("Tapez 'start' pour lancer le jeu ou 'quit' pour quitter\n");

    while(running) {
        printf("Message > ");
        if (fgets(message, BUFFER_SIZE, stdin) == NULL) break;
        
        message[strcspn(message, "\n")] = 0;
        
        if (strcmp(message, "quit") == 0) break;

        if (strcmp(message, "start") == 0) {
            printf("Démarrage du jeu...\n");
            // Le serveur lancera ./proto automatiquement
        }

        if (send(socket_descriptor, message, strlen(message), 0) < 0) {
            perror("Erreur envoi");
            break;
        }

        int bytes_received = recv(socket_descriptor, buffer, BUFFER_SIZE-1, 0);
        if (bytes_received <= 0) {
            printf("Serveur déconnecté\n");
            break;
        }

        buffer[bytes_received] = '\0';
        printf("Réponse serveur: %s\n", buffer);
    }

    printf("Fermeture de la connexion...\n");
    close(socket_descriptor);
    return 0;
}