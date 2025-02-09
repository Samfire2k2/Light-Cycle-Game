#include <stdlib.h>
#include <stdio.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#define TAILLE_MAX_NOM 256

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;

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
}

int main(int argc, char **argv) {
    int socket_descriptor, nouv_socket_descriptor;
    socklen_t longueur_adresse_courante;
    sockaddr_in adresse_locale, adresse_client_courant;
    hostent *ptr_hote;
    char machine[TAILLE_MAX_NOM + 1];

    if (gethostname(machine, TAILLE_MAX_NOM) < 0) {
        perror("erreur : gethostname");
        exit(1);
    }

    if ((ptr_hote = gethostbyname(machine)) == NULL) {
        perror("erreur : impossible de trouver le serveur a partir de son nom.");
        exit(1);
    }

    memset(&adresse_locale, 0, sizeof(adresse_locale));
    bcopy((char *)ptr_hote->h_addr, (char *)&adresse_locale.sin_addr, ptr_hote->h_length);
    adresse_locale.sin_family = ptr_hote->h_addrtype;
    adresse_locale.sin_addr.s_addr = INADDR_ANY;
    adresse_locale.sin_port = htons(5000);

    printf("numero de port pour la connexion au serveur : %d \n", 
           ntohs(adresse_locale.sin_port));

    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("erreur : impossible de creer la socket de connexion avec le client.");
        exit(1);
    }

    int yes = 1;
    if (setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("erreur : impossible de configurer SO_REUSEADDR");
        close(socket_descriptor);
        exit(1);
    }

    if ((bind(socket_descriptor, (sockaddr *)(&adresse_locale), 
             sizeof(adresse_locale))) < 0) {
        perror("erreur : impossible de lier la socket a l'adresse de connexion.");
        close(socket_descriptor);
        exit(1);
    }

    if (listen(socket_descriptor, 5) < 0) {
        perror("erreur : impossible de mettre la socket en écoute");
        close(socket_descriptor);
        exit(1);
    }

    while(1) {
        longueur_adresse_courante = sizeof(adresse_client_courant);

        if ((nouv_socket_descriptor = accept(socket_descriptor, 
            (sockaddr *)(&adresse_client_courant),
            &longueur_adresse_courante)) < 0) {
            perror("erreur : impossible d'accepter la connexion avec le client.");
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("erreur : fork a échoué");
            close(nouv_socket_descriptor);
            continue;
        }
        
        if (pid == 0) { // Child process
            close(socket_descriptor);
            renvoi(nouv_socket_descriptor);
            close(nouv_socket_descriptor);
            exit(0);
        } else { // Parent process
            close(nouv_socket_descriptor);
        }

        pid = fork();
        if (pid < 0) {
            perror("erreur : second fork a échoué");
            continue;
        }
        
        if (pid == 0) { // Child process
            close(socket_descriptor);
            char *args[] = {"./proto", NULL};
            execv(args[0], args);
            perror("erreur : execv a echoue");
            exit(1);
        }
    }

    close(socket_descriptor);
    return 0;
}