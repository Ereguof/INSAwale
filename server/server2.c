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

char *serializeIntArray(char *buffer, int *array, int size)
{
   for (int i = 0; i < size; i++)
   {
      buffer += sprintf(buffer, "%d", array[i]);
      if (i < size - 1)
      {
         buffer += sprintf(buffer, ",");
      }
   }
   return buffer;
}

int afficherPartie(Partie *partie)
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

int afficherParties(Partie *parties, int nbParties)
{
   printf("Liste des parties en cours :\n");
   printf("Nombre de parties : %d\n", nbParties);
   for (int i = 0; i < nbParties; i++)
   {
      if (parties[i].accepted == 1)
      {
         printf("Partie %d\n", i + 1);
         afficherPartie(&parties[i]);
      }
      else
      {
         printf("Partie non acceptée\n");
         afficherPartie(&parties[i]);
      }
   }
}

int afficherClients(Client *clients, int actual)
{
   printf("Liste des clients connectés :\n");
   for (int i = 0; i < actual; i++)
   {
      printf("Client %d : %s\n", i, clients[i].name);
   }
   return 0;
}

int enJeu(Client *client, Partie parties[MAX_PARTIES], int nbParties)
{
   if (client->partie != NULL)
   {
      return 1;
   }
   for (int i = 0; i < nbParties; i++)
   {
      if (parties[i].client1->name == client->name || parties[i].client2->name == client->name)
      {
         return 1;
      }
   }
   return 0;
}

int initPlateau(int plateau[TAILLE_PLATEAU])
{
   for (int i = 0; i < TAILLE_PLATEAU; i++)
   {
      plateau[i] = NB_GRAINES / TAILLE_PLATEAU;
   }
   return 0;
}

int envoyerPlateau(SOCKET sock, int plateau[TAILLE_PLATEAU])
{
   char buffer[BUF_SIZE];
   buffer[0] = 'P';
   serializeIntArray(buffer, plateau, TAILLE_PLATEAU);
   write_client(sock, buffer);
   write_client(sock, "\n");
   return 0;
}

int envoyerScore(SOCKET sock, Partie *partie)
{
   char buffer[BUF_SIZE];
   buffer[0] = 0;
   buffer[0] = 'S';
   strcat(buffer, partie->client1->name);
   strcat(buffer, " : ");
   sprintf(buffer, "%d", partie->client1->nbGraines);
   strcat(buffer, " ");
   strcat(buffer, partie->client2->name);
   strcat(buffer, " : ");
   sprintf(buffer, "%d", partie->client2->nbGraines);
   write_client(sock, buffer);
   return 0;
}

static int command(Partie parties[MAX_PARTIES], Client clients[MAX_CLIENTS], int actual, int *nbParties, Client *client, char *buffer)
{
   afficherClients(clients, actual);
   afficherParties(parties, *nbParties);
   char d[] = " ";
   char *p = strtok(buffer, d); // permet de récupérer le premier mot de la commande

   if (strcmp(p, "/list") == 0)
   {
      write_client(client->sock, "Liste des participants connectés :\n");
      for (int i = 0; i < actual; i++)
      {
         buffer[0] = 0;
         strncat(buffer, clients[i].name, BUF_SIZE - 1);
         strncat(buffer, "\n", BUF_SIZE - strlen(buffer) - 1);
         write_client(client->sock, buffer);
      }
      return 1;
   }

   else if (strcmp(p, "/challenge") == 0)
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
      else if (enJeu(client, parties, *nbParties))
      {
         write_client(client->sock, "Vous êtes déjà en partie\n");
         return 1;
      }
      else
      {
         for (int i = 0; i < actual; i++)
         {
            if (strcmp(p, clients[i].name) == 0 && enJeu(&clients[i], parties, *nbParties) == 0)
            {
               Partie *partie = &parties[*nbParties];
               client->partie = partie;
               clients[i].partie = partie;
               partie->client1 = client;
               partie->client2 = &clients[i];
               partie->nbSpectateurs = 0;
               (*nbParties)++;
               write_client(clients[i].sock, client->name);
               write_client(clients[i].sock, " vous a défié : Acceptez-vous ? (Type '/accept' or '/deny')\n");
               write_client(client->sock, "Défi envoyé\n");
               return 1;
            }
            else if (strcmp(p, clients[i].name) == 0 && enJeu(&clients[i], parties, *nbParties))
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

   else if (strcmp(p, "/accept") == 0)
   {
      if (client->partie != NULL && client->partie->accepted == 0)
      {
         client->partie->accepted = 1;
         client->partie->tour = random() % 2 + 1;
         initPlateau(client->partie->plateau);
         client->nbGraines = 0;
         client->partie->client1->nbGraines = 0;
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
         envoyerPlateau(client->partie->client1->sock, client->partie->plateau);
         envoyerScore(client->partie->client1->sock, client->partie);
         envoyerPlateau(client->sock, client->partie->plateau);
         envoyerScore(client->sock, client->partie);
         return 1;
      }
      else
      {
         write_client(client->sock, "Vous n'avez pas de défi en attente\n");
         return 1;
      }
   }

   else if (strcmp(p, "/deny") == 0)
   {
      if (client->partie != NULL && client->partie->accepted == 0)
      {
         write_client(client->partie->client1->sock, "Défi refusé\n");
         client->partie = NULL;
         for (int i = 0; i < *nbParties; i++)
         {
            if (parties[i].client1 == client || parties[i].client2 == client)
            {
               remove_partie(parties, i, nbParties);
               i--;
            }
         }
      }
      else
      {
         write_client(client->sock, "Vous n'avez pas de défi en attente\n");
      }
   }

   else if (strcmp(p, "/spectate") == 0)
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
      else if (enJeu(client, parties, *nbParties))
      {
         write_client(client->sock, "Vous êtes déjà en partie\n");
         return 1;
      }
      else
      {
         for (int i = 0; i < actual; i++)
         {
            if (strcmp(p, clients[i].name) == 0 && enJeu(&clients[i], parties, *nbParties))
            {
               write_client(client->sock, "Vous observez la partie de ");
               write_client(client->sock, clients[i].name);
               write_client(client->sock, "\n");
               envoyerPlateau(client->sock, clients[i].partie->plateau);
               envoyerScore(client->sock, clients[i].partie);
               clients[i].partie->spectateurs[clients[i].partie->nbSpectateurs] = *client;
               clients[i].partie->nbSpectateurs++;
               return 1;
            }
            else if (strcmp(p, clients[i].name) == 0 && enJeu(&clients[i], parties, *nbParties) == 0)
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
   else if (strcmp(p, "/play") == 0)
   {
      p = strtok(NULL, d);
      if (p == NULL)
      {
         write_client(client->sock, "WARNING : Enter a square between 1 and 6\n");
      }
      else
      {
         p = p[0];
      }
      if (p > '0' && p < '7')
      {
         int square = (int) p;
         printf("la case choisie est %d",square);
      }
   }

   // rajouter les commandes permettant de jouer

   else if (strcmp(p, "/all") == 0)
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
      printf("%s : %s\n", client->name, message);
      return 1;
   }

   else if (strcmp(p, "/mp") == 0)
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

static void app(void)
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

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client c = {csock};
         strncpy(c.name, buffer, BUF_SIZE - 1);
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
               Client * client = &clients[i];
               int c = read_client(clients[i].sock, buffer);

               /* client disconnected */
               if (c == 0)
               {
                  closesocket(clients[i].sock);
                  strncpy(buffer, client->name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, *client, actual, buffer, 1);
                  remove_client(clients, i, &actual, &nbParties, parties);
                  // afficherClients(clients, actual);
                  // afficherParties(parties, nbParties);
               }
               else
               {
                  command(parties, clients, actual, &nbParties, client, buffer);
                  // afficherParties(parties, nbParties);
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for (i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *clients, int to_remove, int *actual, int *nbParties, Partie *parties)
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
      for (int i = 0; i < *nbParties; i++)
      {
         if (parties[i].client1 == &clients[to_remove] || parties[i].client2 == &clients[to_remove])
         {
            remove_partie(parties, i, nbParties);
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
                  remove_client(parties[i].spectateurs, j, &parties[i].nbSpectateurs, 0, 0);
                  j--;
               }
            }
         }
      }
   }
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void remove_partie(Partie *parties, int to_remove, int *nbParties)
{
   parties[to_remove].client1->partie = NULL;
   parties[to_remove].client2->partie = NULL;
   memmove(parties + to_remove, parties + to_remove + 1, (*nbParties - to_remove - 1) * sizeof(Partie));
   (*nbParties)--;
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
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
