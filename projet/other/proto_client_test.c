#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define TAILLE_MAX_NOM 256

int main(int argc, char **argv) {
    int socket_descriptor;
    struct sockaddr_in adresse_serveur;
    char buffer[256];
    char *message = "Bonjour, serveur!";
    
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("erreur : impossible de creer la socket de connexion avec le serveur.");
        exit(1);
    }

    adresse_serveur.sin_family = AF_INET;
    adresse_serveur.sin_port = htons(5000);
    adresse_serveur.sin_addr.s_addr = inet_addr("127.0.0.1"); // Adresse IP du serveur (localhost)

    if (connect(socket_descriptor, (struct sockaddr *)&adresse_serveur, sizeof(adresse_serveur)) < 0) {
        perror("erreur : impossible de se connecter au serveur.");
        close(socket_descriptor);
        exit(1);
    }

    printf("Connexion etablie avec le serveur.\n");

    send(socket_descriptor, message, strlen(message), 0);
    printf("Message envoye au serveur: %s\n", message);

    int longueur = read(socket_descriptor, buffer, sizeof(buffer)-1);
    if (longueur > 0) {
        buffer[longueur] = '\0';
        printf("Reponse du serveur : %s\n", buffer);
    } else {
        printf("Aucune reponse du serveur.\n");
    }

    close(socket_descriptor);
    return 0;
}