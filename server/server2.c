#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server2.h"
#include "client2.h"

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if (err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// FONCTIONS DE JEU
//////////////////////////////////////////////////////////////////////////////////////////////////////////

char *showBoard(int plateau[], int num_joueur_appelant, char *message)
{
   message[0] = 0;

   char temp[12];
   strcat(message, "Plateau : pour joueur ");
   sprintf(temp, "%d", num_joueur_appelant);
   strcat(message, temp);
   strcat(message, "\n");

   if (num_joueur_appelant == 1)
   {
      for (int i = TAILLE_PLATEAU - 1; i > TAILLE_PLATEAU / 2 - 1; i--)
      {
         sprintf(temp, "%0*d ", 2, plateau[i]);
         strcat(message, temp);
      }
      strcat(message, "\n");
      for (int i = 0; i < TAILLE_PLATEAU / 2; i++)
      {
         sprintf(temp, "%0*d ", 2, plateau[i]);
         strcat(message, temp);
      }
   }
   else
   {
      for (int i = (TAILLE_PLATEAU / 2) - 1; i >= 0; i--)
      {
         sprintf(temp, "%0*d ", 2, plateau[i]);
         strcat(message, temp);
      }
      strcat(message, "\n");
      for (int i = TAILLE_PLATEAU / 2; i < TAILLE_PLATEAU; i++)
      {
         sprintf(temp, "%0*d ", 2, plateau[i]);
         strcat(message, temp);
      }
      strcat(message, "\n");
   }
   return message;
}

int emptyEnemy(int plateau[], Client *client)
{
   if (client->numJoueur == 2)
   {
      for (int i = 0; i < TAILLE_PLATEAU / 2; i++)
      {
         if (plateau[i] != 0)
         {
            return 0;
         }
      }
   }
   else if (client->numJoueur == 1)
   {
      for (int i = TAILLE_PLATEAU / 2; i < TAILLE_PLATEAU; i++)
      {
         if (plateau[i] != 0)
         {
            return 0;
         }
      }
   }
   return 1;
}

int validPlay(int plateau[], Client *client, int case_joueur)
{
   if (case_joueur < 1 || case_joueur > TAILLE_PLATEAU / 2)
   {
      printf("Case invalide : choisis le bon nombre\n");
      return 0;
   }
   if (client->numJoueur == 1)
   {
      if (emptyEnemy(plateau, client))
      {
         if (TAILLE_PLATEAU / 2 - case_joueur + 1 > plateau[case_joueur - 1])
         {
            write_client(client, "Case invalide : famine\n");
            return 0;
         }
      }
      if (plateau[case_joueur - 1] == 0)
      {
         write_client(client, "Case vide\n");
         return 0;
      }
   }
   else if (client->numJoueur == 2)
   {
      if (emptyEnemy(plateau, client))
      {
         if (TAILLE_PLATEAU / 2 - case_joueur + 1 > plateau[case_joueur - 1 + TAILLE_PLATEAU / 2])
         {
            write_client(client, "Case invalide : famine\n");
            return 0;
         }
      }
      if (plateau[case_joueur + 5] == 0)
      {
         write_client(client, "Case vide\n");
         return 0;
      }
   }
   return 1;
}

int nextPlay(int plateau[], Client *client, int case_joueur)
{
   case_joueur--;
   if (client->numJoueur == 2)
   {
      case_joueur += TAILLE_PLATEAU / 2;
   }
   int nb_graines = plateau[case_joueur];
   plateau[case_joueur] = 0;
   int i = case_joueur + 1;
   while (nb_graines > 0)
   {
      if (i != case_joueur)
      {
         plateau[i % TAILLE_PLATEAU]++;
         nb_graines--;
      }
      i++;
   }
   i--;

   int savePlateau[TAILLE_PLATEAU];
   for (int i = 0; i < TAILLE_PLATEAU; i++)
   {
      savePlateau[i] = plateau[i];
   }
   int saveGraines = client->nbGraines;

   if (client->numJoueur == 1)
   {
      while ((i % TAILLE_PLATEAU > TAILLE_PLATEAU / 2 - 1) && (plateau[i % TAILLE_PLATEAU] == 2 || plateau[i % TAILLE_PLATEAU] == 3))
      {
         client->nbGraines += plateau[i % TAILLE_PLATEAU];
         plateau[i % TAILLE_PLATEAU] = 0;
         i--;
      }
   }
   else if (client->numJoueur == 2)
   {
      while ((i % TAILLE_PLATEAU < TAILLE_PLATEAU / 2) && (plateau[i % TAILLE_PLATEAU] == 2 || plateau[i % TAILLE_PLATEAU] == 3))
      {
         client->nbGraines += plateau[i % TAILLE_PLATEAU];
         plateau[i % TAILLE_PLATEAU] = 0;
         i--;
      }
   }

   if (emptyEnemy(plateau, client))
   {
      for (int i = 0; i < TAILLE_PLATEAU; i++)
      {
         plateau[i] = savePlateau[i];
         client->nbGraines = saveGraines;
      }
   }
   return i;
}

int endGame(Client *client1, Client *client2)
{
   if (client1->nbGraines >= NB_GRAINES_WIN)
   {
      return 1;
   }
   else if (client2->nbGraines >= NB_GRAINES_WIN)
   {
      return 2;
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// COMMANDES DE JEU
//////////////////////////////////////////////////////////////////////////////////////////////////////////

static int command(Partie parties[MAX_PARTIES], Client clients[MAX_CLIENTS], int actual, int *nbParties, Client *clientCurr, char *buffer) // permet de traiter les commandes des clients
{
   char d[] = " ";
   char *p = strtok(buffer, d); // permet de récupérer le premier mot de la commande
   Client *client = clientCurr;

   if (strcmp(p, "/help") == 0) // permet d'afficher les commandes disponibles
   {
      write_client(client->sock, "Liste des commandes disponibles :\n");
      write_client(client->sock, "/help : Permet d'afficher les commandes disponibles\n");
      write_client(client->sock, "/setbio [biographie] : Permet de définir une biographie\n");
      write_client(client->sock, "/bio [pseudo] : Permet d'afficher la biographie d'un joueur\n");
      write_client(client->sock, "/list : Permet de lister les participants connectés\n");
      write_client(client->sock, "/games : Permet de lister les parties en cours\n");
      write_client(client->sock, "/challenge [pseudo] : Permet de défier un joueur\n");
      write_client(client->sock, "/accept : Permet d'accepter un défi\n");
      write_client(client->sock, "/deny : Permet de refuser un défi\n");
      write_client(client->sock, "/spectate [pseudo] : Permet de spectater une partie\n");
      write_client(client->sock, "/play [case] : Permet de jouer un coup\n");
      write_client(client->sock, "/out : Permet de quitter une partie, que ce soit en tant que joueur ou spectateur\n");
      write_client(client->sock, "/all [message] : Permet d'envoyer un message à tous les participants\n");
      write_client(client->sock, "/mp [player] [message] : Permet d'envoyer un message privé a un joueur\n");
      return 1;
   }

   else if (strcmp(p, "/setbio") == 0)
   {
      p = strtok(NULL, d);
      if (p == NULL)
      {
         write_client(client->sock, "Veuillez entrer une biographie\n");
         return 1;
      }
      char message[BUF_SIZE / 2];
      message[0] = 0;
      while (p != NULL)
      {
         strcat(message, p);
         strcat(message, " ");
         p = strtok(NULL, d);
      }
      strncpy(client->bio, message, BUF_SIZE / 2 - 1);
      write_client(client->sock, "Biographie mise à jour\n");
      return 1;
   }

   else if (strcmp(p, "/bio") == 0) // permet d'afficher la biographie du joueur choisi
   {
      p = strtok(NULL, d);
      if (p == NULL)
      {
         write_client(client->sock, "Veuillez entrer le nom du joueur\n");
         return 1;
      }
      for (int i = 0; i < actual; i++)
      {
         if (strcmp(p, clients[i].name) == 0)
         {
            write_client(client->sock, "Biographie : \n");
            write_client(client->sock, clients[i].bio);
            return 1;
         }
      }
      write_client(client->sock, "Le joueur n'existe pas\n");
      return 1;
   }

   if (strcmp(p, "/list") == 0)
   {
      write_client(client->sock, "Liste des participants connectés :\n");
      for (int i = 0; i < actual; i++)
      {
         buffer[0] = 0;
         strcat(buffer, clients[i].name);
         strcat(buffer, "\n");
         write_client(client->sock, buffer);
      }
      return 1;
   }

   if (strcmp(p, "/games") == 0)
   {
      write_client(client->sock, "Liste des parties en cours :\n\n");
      for (int i = 0; i < *nbParties; i++)
      {
         buffer[0] = 0;
         strncat(buffer, parties[i].client1->name, BUF_SIZE - 1);
         strncat(buffer, " vs ", BUF_SIZE - strlen(buffer) - 1);
         strncat(buffer, parties[i].client2->name, BUF_SIZE - strlen(buffer) - 1);
         strncat(buffer, "\n", BUF_SIZE - strlen(buffer) - 1);
         strncat(buffer, "Score : ", BUF_SIZE - strlen(buffer) - 1);

         char scoreBuffer[12];
         sprintf(scoreBuffer, "%d", parties[i].client1->nbGraines);
         strncat(buffer, scoreBuffer, BUF_SIZE - strlen(buffer) - 1);

         strncat(buffer, " - ", BUF_SIZE - strlen(buffer) - 1);

         sprintf(scoreBuffer, "%d", parties[i].client2->nbGraines);
         strncat(buffer, scoreBuffer, BUF_SIZE - strlen(buffer) - 1);

         strncat(buffer, "\n\n", BUF_SIZE - strlen(buffer) - 1);
         write_client(client->sock, buffer);
      }
      return 1;
   }

   else if (strcmp(p, "/challenge") == 0) // permet de défier un joueur
   {
      p = strtok(NULL, d);
      if (p == NULL)
      {
         write_client(client->sock, "Veuillez entrer le nom du joueur à défier\n");
         return 1;
      }
      else if (strcmp(p, client->name) == 0)
      {
         write_client(client->sock, "Vous ne pouvez pas vous défier vous-même\n");
         return 1;
      }
      else if (inGame(client) || inSpectate(client, parties, *nbParties))
      {
         write_client(client->sock, "Vous êtes déjà en partie ou spectateur\n");
         return 1;
      }
      else
      {
         for (int i = 0; i < actual; i++)
         {
            if (strcmp(p, clients[i].name) == 0 && inGame(&clients[i]) == 0)
            {
               Partie *partie = &parties[*nbParties];
               client->partie = partie;
               clients[i].partie = partie;
               partie->client1 = client;
               partie->client2 = &clients[i];
               partie->nbSpectateurs = 0;
               partie->accepted = 0;
               (*nbParties)++;
               write_client(clients[i].sock, client->name);
               write_client(clients[i].sock, " vous a défié : Acceptez-vous ? (Type '/accept' or '/deny')\n");
               write_client(client->sock, "Défi envoyé\n");
               return 1;
            }
            else if (strcmp(p, clients[i].name) == 0 && inGame(&clients[i]))
            {
               write_client(client->sock, "Le joueur est déjà en partie\n");
               return 1;
            }
         }
         write_client(client->sock, "Le joueur n'existe pas\n");
         return 1;
      }
      return 1;
   }

   else if (strcmp(p, "/accept") == 0) // permet d'accepter un défi
   {
      if (client->partie != NULL && client->partie->accepted == 0 && inSpectate(client, parties, *nbParties) == 0 && client->partie->client2 == client)
      {
         client->partie->accepted = 1;
         client->partie->tour = random() % 2 + 1;
         initBoard(client->partie->plateau);
         client->partie->client2->nbGraines = 0;
         client->partie->client1->nbGraines = 0;
         client->partie->client1->numJoueur = 1;
         client->partie->client2->numJoueur = 2;
         write_client(client->sock, "Défi accepté\n");
         write_client(client->partie->client1->sock, "Défi accepté\n");
         if (client->partie->tour == 1)
         {
            write_client(client->partie->client1->sock, "Vous commencez\n");
            write_client(client->sock, "Votre adversaire commence\n");
         }
         else
         {
            write_client(client->sock, "Vous commencez\n");
            write_client(client->partie->client1->sock, "Votre adversaire commence\n");
         }
         sendBoard(client->partie->client1->sock, client->partie->plateau, client->partie->client1->numJoueur);
         sendScore(client->partie->client1->sock, client->partie);
         sendBoard(client->sock, client->partie->plateau, client->partie->client2->numJoueur);
         sendScore(client->sock, client->partie);
         return 1;
      }
      else if (client->partie != NULL && client->partie->accepted == 1)
      {
         write_client(client->sock, "Vous avez déjà accepté le défi\n");
         return 1;
      }
      else if (inSpectate(client, parties, *nbParties))
      {
         write_client(client->sock, "Vous êtes en mode spectateur, veuillez d'abord le quitter (/quit)\n");
         return 1;
      }
      else
      {
         write_client(client->sock, "Vous n'avez pas de défi en attente\n");
         return 1;
      }
   }

   else if (strcmp(p, "/deny") == 0) // permet de refuser un défi
   {
      if (client->partie != NULL && client->partie->accepted == 0 && inSpectate(client, parties, *nbParties) == 0)
      {
         write_client(client->partie->client1->sock, "Défi refusé\n");
         client->partie = NULL;
         for (int i = 0; i < *nbParties; i++)
         {
            if (parties[i].client1 == client || parties[i].client2 == client)
            {
               remove_game(parties, i, nbParties);
               i--;
            }
         }
      }
      else if (client->partie != NULL && client->partie->accepted == 1)
      {
         write_client(client->sock, "Vous avez déjà accepté le défi\n");
         return 1;
      }
      else if (inSpectate(client, parties, *nbParties))
      {
         write_client(client->sock, "Vous êtes en mode spectateur, veuillez d'abord le quitter (/quit)\n");
         return 1;
      }
      else
      {
         write_client(client->sock, "Vous n'avez pas de défi en attente\n");
      }
   }

   else if (strcmp(p, "/spectate") == 0) // permet de spectater une partie
   {
      p = strtok(NULL, d);
      if (p == NULL)
      {
         write_client(client->sock, "WARNING : Veuillez entrer le nom du joueur à observer (Utilisation : /challenge [pseudo])\n");
         return 1;
      }
      else if (strcmp(p, client->name) == 0)
      {
         write_client(client->sock, "Vous ne pouvez pas vous observer vous-même\n");
         return 1;
      }
      else if (inGame(client) || inSpectate(client, parties, *nbParties))
      {
         write_client(client->sock, "Vous êtes déjà en partie ou spectateur\n");
         return 1;
      }
      else
      {
         for (int i = 0; i < actual; i++)
         {
            if (strcmp(p, clients[i].name) == 0 && inGame(&clients[i]))
            {
               char message[BUF_SIZE];
               message[0] = 0;
               strncat(message, "Vous observez la partie de ", BUF_SIZE - 1);
               strncat(message, clients[i].name, BUF_SIZE - strlen(message) - 1);
               strncat(message, "\n", BUF_SIZE - strlen(message) - 1);
               write_client(client->sock, message);
               sendBoard(client->sock, clients[i].partie->plateau, 1);
               sendScore(client->sock, clients[i].partie);
               clients[i].partie->spectateurs[clients[i].partie->nbSpectateurs] = *client;
               clients[i].partie->nbSpectateurs++;
               return 1;
            }
            else if (strcmp(p, clients[i].name) == 0 && inGame(&clients[i]) == 0)
            {
               write_client(client->sock, "Le joueur n'est pas en partie\n");
               return 1;
            }
         }
         write_client(client->sock, "Le joueur n'existe pas\n");
         return 1;
      }
      return 1;
   }

   else if (strcmp(p, "/play") == 0) // permet de jouer un coup
   {
      if (client->partie == NULL)
      {
         write_client(client->sock, "Vous n'êtes pas en partie\n");
         return 1;
      }
      else if (client->partie->accepted == 0)
      {
         write_client(client->sock, "La partie n'a pas encore été acceptée\n");
         return 1;
      }
      else if (client->partie->tour != client->numJoueur)
      {
         write_client(client->sock, "Ce n'est pas votre tour\n");
         return 1;
      }
      else
      {
         int square = 0;
         p = strtok(NULL, d);
         if (p == NULL)
         {
            write_client(client->sock, "WARNING : Enter a square between 1 and 6\n");
         }
         else
         {
            square = atoi(p);
         }
         if (square > 0 && square < 7)
         {
            if (validPlay(client->partie->plateau, client, square))
            {
               char message[BUF_SIZE];
               message[0] = 0;
               strcat(message, "La case choisie est : ");
               strcat(message, p);
               strcat(message, "\n");
               write_client(client->partie->client1->sock, message);
               write_client(client->partie->client2->sock, message);
               nextPlay(client->partie->plateau, client, square);
               sendBoard(client->partie->client1->sock, client->partie->plateau, client->partie->client1->numJoueur);
               sendScore(client->partie->client1->sock, client->partie);
               sendBoard(client->partie->client2->sock, client->partie->plateau, client->partie->client2->numJoueur);
               sendScore(client->partie->client2->sock, client->partie);

               if (client->partie->tour == 1)
               {
                  client->partie->tour = 2;
               }
               else if (client->partie->tour == 2)
               {
                  client->partie->tour = 1;
               }
               int res = endGame(client->partie->client2, client->partie->client1);

               if (res == 1)
               {
                  write_client(client->partie->client1->sock, "Vous avez GAGNÉ !");
                  write_client(client->partie->client2->sock, "Vous avez PERDU !");
               }
               if (res == 2)
               {
                  write_client(client->partie->client2->sock, "Vous avez GAGNÉ !");
                  write_client(client->partie->client1->sock, "Vous avez PERDU !");
               }
            }
         }
         else
         {
            write_client(client->sock, "WARNING : Enter a square between 1 and 6\n");
         }
      }
   }

   // rajouter les commandes permettant de jouer

   else if (strcmp(p, "/out") == 0) // permet de quitter une partie, que ce soit en tant que joueur ou spectateur
   {
      if (client->partie != NULL)
      {
         if (client->partie->client1 == client)
         {
            write_client(client->partie->client2->sock, "L'adversaire s'est déconnecté\n");
            client->partie->client2->partie = NULL;
         }
         else
         {
            write_client(client->partie->client1->sock, "L'adversaire s'est déconnecté\n");
            client->partie->client1->partie = NULL;
         }
         for (int i = 0; i < *nbParties; i++)
         {
            if (parties[i].client1 == client || parties[i].client2 == client)
            {
               remove_game(parties, i, nbParties);
               i--;
            }
         }
         client->partie = NULL;
         write_client(client->sock, "Vous avez quitté la partie\n");
         return 1;
      }
      else if (inSpectate(client, parties, *nbParties))
      {
         for (int i = 0; i < *nbParties; i++)
         {
            for (int j = 0; j < parties[i].nbSpectateurs; j++)
            {
               if (strcmp(parties[i].spectateurs[j].name, client->name) == 0)
               {
                  remove_spectator(parties[i].spectateurs, &parties[i].nbSpectateurs, client);
               }
            }
         }
         write_client(client->sock, "Vous avez cessé de spectater\n");
         return 1;
      }
      else
      {
         write_client(client->sock, "Vous n'êtes pas en partie ou spectateur\n");
         return 1;
      }
   }

   else if (strcmp(p, "/all") == 0) // permet d'envoyer un message à tous les participants
   {
      p = strtok(NULL, d);
      if (p == NULL)
      {
         write_client(client->sock, "Veuillez entrer un message\n");
         return 1;
      }
      char message[BUF_SIZE];
      message[0] = 0;
      while (p != NULL)
      {
         strncat(message, p, BUF_SIZE - strlen(message) - 1);
         strncat(message, " ", BUF_SIZE - strlen(message) - 1);
         p = strtok(NULL, d);
      }
      send_message_to_all_clients(clients, *client, actual, message, 0);
      return 1;
   }

   else if (strcmp(p, "/mp") == 0) // permet d'envoyer un message privé à un participant
   {
      p = strtok(NULL, d);
      if (p == NULL)
      {
         write_client(client->sock, "Veuillez entrer le nom du joueur à qui envoyer un message privé\n");
         return 1;
      }
      char message[BUF_SIZE];
      message[0] = 0;
      strncat(message, client->name, BUF_SIZE - 1);
      strncat(message, " (mp) : ", BUF_SIZE - strlen(message) - 1);
      char nom[BUF_SIZE];
      strcpy(nom, p);
      p = strtok(NULL, d);
      if (p == NULL)
      {
         write_client(client->sock, "Veuillez entrer un message\n");
         return 1;
      }
      while (p != NULL)
      {
         strncat(message, p, BUF_SIZE - strlen(message) - 1);
         strncat(message, " ", BUF_SIZE - strlen(message) - 1);
         p = strtok(NULL, d);
      }
      for (int i = 0; i < actual; i++)
      {
         if (strcmp(clients[i].name, nom) == 0)
         {
            write_client(clients[i].sock, message);
            return 1;
         }
      }
      write_client(client->sock, "Le joueur n'existe pas\n");
      return 1;
   }

   else
   {
      write_client(client->sock, "Commande inconnue\n");
      return 1;
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// FONCTIONS UTILITAIRES
//////////////////////////////////////////////////////////////////////////////////////////////////////////

int showGame(Partie *partie) // permet d'afficher l'état d'une partie
{
   printf("Client 1 : %s\n", partie->client1->name);
   printf("Client 2 : %s\n", partie->client2->name);
   printf("Plateau : ");
   for (int i = 0; i < TAILLE_PLATEAU; i++)
   {
      printf("%d ", partie->plateau[i]);
   }
   printf("\n");
   printf("Accepted : %d\n", partie->accepted);
   printf("Tour : %d\n", partie->tour);
   printf("Nombre de spectateurs : %d\n", partie->nbSpectateurs);
   printf("Spectateurs : ");
   for (int i = 0; i < partie->nbSpectateurs; i++)
   {
      printf("%s\n", partie->spectateurs[i].name);
   }
   return 0;
}

int showGames(Partie *parties, int nbParties) // permet d'afficher la liste des parties en cours
{
   printf("Liste des parties en cours :\n");
   printf("Nombre de parties : %d\n", nbParties);
   for (int i = 0; i < nbParties; i++)
   {
      if (parties[i].accepted == 1)
      {
         printf("Partie %d\n", i + 1);
         showGame(&parties[i]);
      }
      else
      {
         printf("Partie non acceptée\n");
         showGame(&parties[i]);
      }
   }
}

int showClients(Client *clients, int actual) // permet d'afficher la liste des clients connectés
{
   printf("Liste des clients connectés :\n");
   for (int i = 0; i < actual; i++)
   {
      printf("Client %d : %s\n", i, clients[i].name);
   }
   return 0;
}

int inGame(Client *client) // permet de savoir si un client est en partie
{
   if (client->partie != NULL)
   {
      return 1;
   }
   return 0;
}

int inSpectate(Client *client, Partie parties[MAX_PARTIES], int nbParties) // permet de savoir si un client est en mode spectateur
{
   for (int i = 0; i < nbParties; i++)
   {
      for (int j = 0; j < parties[i].nbSpectateurs; j++)
      {
         if (strcmp(parties[i].spectateurs[j].name, client->name) == 0)
         {
            return 1;
         }
      }
   }
   return 0;
}

int initBoard(int plateau[TAILLE_PLATEAU]) // permet d'initialiser le plateau de jeu
{
   for (int i = 0; i < TAILLE_PLATEAU; i++)
   {
      plateau[i] = NB_GRAINES / TAILLE_PLATEAU;
   }
   return 0;
}

int sendBoard(SOCKET sock, int plateau[TAILLE_PLATEAU], int numPlayer) // permet d'envoyer le plateau de jeu à un client
{
   char message[BUF_SIZE];
   showBoard(plateau, numPlayer, message);
   write_client(sock, message);
   write_client(sock, "\n");
   return 0;
}

int sendScore(SOCKET sock, Partie *partie) // permet d'envoyer le score d'une partie à un client
{
   char nbGraines[12];
   sprintf(nbGraines, "%d", partie->client1->nbGraines);
   char message[BUF_SIZE];
   message[0] = 0;
   strcat(message, partie->client1->name);
   strcat(message, " : ");
   strcat(message, nbGraines);
   sprintf(nbGraines, "%d", partie->client2->nbGraines);
   strcat(message, "    ");
   strcat(message, partie->client2->name);
   strcat(message, " : ");
   strcat(message, nbGraines);
   strcat(message, "\n");
   write_client(sock, message);
   return 0;
}

static void remove_spectator(Client *spectateurs, int *nbSpectateurs, Client *client) // permet de retirer un spectateur de la liste des spectateurs
{
   for (int i = 0; i < *nbSpectateurs; i++)
   {
      if (strcmp(spectateurs[i].name, client->name) == 0)
      {
         memmove(spectateurs + i, spectateurs + i + 1, (*nbSpectateurs - i - 1) * sizeof(Client));
         (*nbSpectateurs)--;
      }
   }
}

static void remove_client(Client *clients, int to_remove, int *actual, int *nbParties, Partie *parties) // permet de retirer un client de la liste des clients
{
   if (clients[to_remove].partie != NULL)
   {
      if (clients[to_remove].partie->client1 == &clients[to_remove])
      {
         write_client(clients[to_remove].partie->client2->sock, "L'adversaire s'est déconnecté\n");
         clients[to_remove].partie->client2->partie = NULL;
      }
      else
      {
         write_client(clients[to_remove].partie->client1->sock, "L'adversaire s'est déconnecté\n");
         clients[to_remove].partie->client1->partie = NULL;
      }
   }
   for (int i = 0; i < *nbParties; i++)
   {
      if (parties[i].client1 == &clients[to_remove] || parties[i].client2 == &clients[to_remove])
      {
         remove_game(parties, i, nbParties);
         i--;
      }
      else
      {
         if (parties[i].client1 > &clients[to_remove])
         {
            parties[i].client1--;
         }
         if (parties[i].client2 > &clients[to_remove])
         {
            parties[i].client2--;
         }
         for (int j = 0; j < parties[i].nbSpectateurs; j++)
         {
            if (strcmp(parties[i].spectateurs[j].name, clients[to_remove].name) == 0)
            {
               remove_spectator(parties[i].spectateurs, &parties[i].nbSpectateurs, &clients[to_remove]);
            }
         }
      }
   }

   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void remove_game(Partie *parties, int to_remove, int *nbParties) // permet de retirer une partie de la liste des parties
{
   parties[to_remove].client1->partie = NULL;
   parties[to_remove].client2->partie = NULL;
   memmove(parties + to_remove, parties + to_remove + 1, (*nbParties - to_remove - 1) * sizeof(Partie));
   (*nbParties)--;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// FONCTIONS DE RÉSEAU
//////////////////////////////////////////////////////////////////////////////////////////////////////////

static void app(void) // permet de gérer les connexions des clients
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE];
   /* the index for the array */
   int actual = 0;
   int max = sock;
   /* an array for all clients */
   Client clients[MAX_CLIENTS];
   Partie parties[MAX_PARTIES];
   int nbParties = 0;

   fd_set rdfs;

   while (1)
   {
      int i = 0;
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for (i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if (FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if (FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = {0};
         size_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if (csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its name */
         if (read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }

         int invalide = 0;
         for (int i = 0; i < actual; i++)
         {
            if (strcmp(clients[i].name, buffer) == 0)
            {
               write_client(csock, "Ce nom est déjà utilisé\n");
               closesocket(csock);
               invalide++;
               break;
            }
         }
         if (invalide)
         {
            continue;
         }
         else
         {
            write_client(csock, "Bienvenue sur le serveur de jeu INSAwalé !\nVoici la liste des commandes disponibles :\n/help : Permet d'afficher les commandes disponibles\n/setbio [biographie] : Permet de définir une biographie\n/bio [pseudo] : Permet d'afficher la biographie d'un joueur\n/list : affiche la liste des participants connectés\n/games : affiche la liste des parties en cours\n/challenge [pseudo] : défie un joueur\n/accept : accepte un défi\n/deny : refuse un défi\n/spectate [pseudo] : observe une partie\n/play [case] : joue un coup\n/out : quitte une partie ou le mode spectateur\n/all [message] : envoie un message à tous les participants\n/mp [pseudo] [message] : envoie un message privé à un participant\nCTRL-C : quitte le serveur\n");
         }

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client c = {csock};
         strncpy(c.name, buffer, BUF_SIZE - 1);
         c.nbGraines = 0;
         c.partie = NULL;
         c.numJoueur = 0;
         c.bio[0] = 0;
         clients[actual] = c;
         actual++;
      }
      else
      {
         int i = 0;
         for (i = 0; i < actual; i++)
         {
            /* a client is talking */
            if (FD_ISSET(clients[i].sock, &rdfs))
            {
               Client *clientCurr = &clients[i];
               int c = read_client(clients[i].sock, buffer);

               /* client disconnected */
               if (c == 0)
               {
                  closesocket(clients[i].sock);
                  strncpy(buffer, clientCurr->name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !\n", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, *clientCurr, actual, buffer, 1);
                  remove_client(clients, i, &actual, &nbParties, parties);
               }
               else
               {
                  command(parties, clients, actual, &nbParties, clientCurr, buffer);
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}

static void clear_clients(Client *clients, int actual) // permet de déconnecter tous les clients
{
   int i = 0;
   for (i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server) // permet d'envoyer un message à tous les participants
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for (i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if (sender.sock != clients[i].sock)
      {
         if (from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
      message[0] = 0;
   }
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   const int enable = 1;
   setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)); // Allow to reuse the port immediately after the server is closed
   SOCKADDR_IN sin = {0};

   if (sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if (bind(sock, (SOCKADDR *)&sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if (send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
