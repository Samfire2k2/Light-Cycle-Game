1. le serv stock une grille par joueur dans laquelle il garde la trace du coup et la position
2. Et à chaque "tour" de jeu, les clients font leur déplacement
3. Envoie le déplacement au serv et le serv check si il y a une collision

(version N clients/1 serveur, on peut utiliser "fork" ou "threads")

mettre nb joueurs

puis lancer le jeu

séquentialiser les actions

2 threads par client :

- un pour lire
- un pour qui envoie la grille pour chaque client (à ce moment-là, les prioriser)

envoyer mail prof pour avoir exemple threads
(thread après le accept)