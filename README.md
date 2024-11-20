# Projet de programmation réseau : INSAwalé

L’objectif du TP est de réaliser un serveur de jeu Awalé. 

Le but est d’avoir une application client/serveur permettant aux clients de jouer des parties, de vérifier que les règles sont bien appliquées, de communiquer, de garder des historiques (de score, de parties jouées, etc).

## Bienvenue sur le serveur de jeu INSAwalé !

Compilation et lancement : 

Il suffit de faire `make` à la racine du projet !

Pour lancer le serveur : `./server2`

Pour lancer un client : `./client2 [IP_SERVER] [PSEUDO]`


### Voici la liste des commandes et fonctionnalités disponibles :

/help : Permet d'afficher les commandes disponibles

/setbio [biographie] : Permet de définir une biographie

/bio [pseudo] : Permet d'afficher la biographie d'un joueur

/list : affiche la liste des participants connectés

/games : affiche la liste des parties en cours

/challenge [pseudo] : défie un joueur

/accept : accepte un défi

/deny : refuse un défi

/spectate [pseudo] : observe une partie

/play [case] : joue un coup

/out : quitte une partie ou le mode spectateur

/all [message] : envoie un message à tous les participants

/mp [pseudo] [message] : envoie un message privé à un participant

CTRL-C : quitte le serveur

### Les règles du jeu peuvent être vues [ici](https://fr.wikipedia.org/wiki/Awal%C3%A9)
