#include <stdlib.h>
#include <stdio.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define TAILLE_MAX_NOM 256
#define PORT 5000
#define MAX_CLIENTS 10

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;

void handle_sigchld(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void renvoi(int sock) {
    char buffer[256];
    int longueur;

    while ((longueur = read(sock, buffer, sizeof(buffer))) > 0) {
        printf("Client %d - message reçu : %s\n", getpid(), buffer);
        
        buffer[0] = 'R';
        buffer[1] = 'E';
        buffer[longueur] = '#';
        buffer[longueur + 1] = '\0';

        printf("Client %d - message traité : %s\n", getpid(), buffer);
        write(sock, buffer, strlen(buffer) + 1);
    }

    printf("Client %d déconnecté\n", getpid());
}

int main(int argc, char **argv) {
    int socket_descriptor;
    socklen_t longueur_adresse_courante;
    sockaddr_in adresse_locale, adresse_client_courant;
    hostent *ptr_hote;
    char machine[TAILLE_MAX_NOM + 1];

    signal(SIGCHLD, handle_sigchld);

    if (gethostname(machine, TAILLE_MAX_NOM) < 0) {
        perror("Erreur gethostname");
        exit(1);
    }

    if ((ptr_hote = gethostbyname(machine)) == NULL) {
        perror("Erreur gethostbyname");
        exit(1);
    }

    memset(&adresse_locale, 0, sizeof(adresse_locale));
    adresse_locale.sin_family = AF_INET;
    adresse_locale.sin_addr.s_addr = INADDR_ANY;
    adresse_locale.sin_port = htons(PORT);

    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erreur création socket");
        exit(1);
    }

    int yes = 1;
    if (setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("Erreur setsockopt");
        close(socket_descriptor);
        exit(1);
    }

    if (bind(socket_descriptor, (sockaddr *)&adresse_locale, sizeof(adresse_locale)) < 0) {
        perror("Erreur bind");
        close(socket_descriptor);
        exit(1);
    }

    if (listen(socket_descriptor, MAX_CLIENTS) < 0) {
        perror("Erreur listen");
        close(socket_descriptor);
        exit(1);
    }

    printf("Serveur démarré sur le port %d\n", PORT);

    while(1) {
        int nouv_socket_descriptor;
        longueur_adresse_courante = sizeof(adresse_client_courant);

        if ((nouv_socket_descriptor = accept(socket_descriptor, 
            (sockaddr *)&adresse_client_courant, 
            &longueur_adresse_courante)) < 0) {
            perror("Erreur accept");
            continue;
        }

        // Premier fork pour la communication client
        pid_t pid = fork();
        if (pid < 0) {
            perror("Erreur fork client");
            close(nouv_socket_descriptor);
            continue;
        }
        
        if (pid == 0) {  // Premier processus fils
            close(socket_descriptor);
            printf("Nouveau client connecté (PID: %d)\n", getpid());
            renvoi(nouv_socket_descriptor);
            close(nouv_socket_descriptor);
            exit(0);
        } else {  // Processus père
            close(nouv_socket_descriptor);

            // Deuxième fork pour lancer proto
            pid_t pid2 = fork();
            if (pid2 < 0) {
                perror("Erreur fork proto");
                continue;
            }
            
            if (pid2 == 0) {  // Deuxième processus fils
                if (execl("./proto", "./proto", NULL) == -1) {
                    perror("Erreur lancement proto");
                    exit(1);
                }
            }
        }
    }

    close(socket_descriptor);
    return 0;
}