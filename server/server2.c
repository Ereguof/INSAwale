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

static int command(Partie parties[MAX_PARTIES], Client clients[MAX_CLIENTS], int actual, int *nbParties, Client *client, char *buffer)
{
   char d[] = " ";
   char *p = strtok(buffer, d); // permet de récupérer le premier mot de la commande

   if (strcmp(p, "Liste") == 0)
   {
      write_client(client->sock, "Liste des participants connectés :\n");
      for (int i = 0; i < actual; i++)
      {
         write_client(client->sock, clients[i].name);
         write_client(client->sock, "\n");
      }
      return 1;
   } 
   
   else if (strcmp(p, "Challenge") == 0)
   {
      p = strtok(NULL, d);
      if (p == NULL)
      {
         write_client(client->sock, "Veuillez entrer le nom du joueur à défier\n");
         return 1;
      } else if (strcmp(p, client->name) == 0)
      {
         write_client(client->sock, "Vous ne pouvez pas vous défier vous-même\n");
         return 1;
      } else
      {
         for (int i = 0; i < actual; i++)
         {
            if (strcmp(p, clients[i].name) == 0)
            {
               Partie partie;
               partie.client1 = client;
               partie.client2 = &clients[i];
               partie.accepted = 0;
               parties[*nbParties] = partie;
               (*nbParties)++;
               write_client(clients[i].sock, strcat(client->name, " vous a défié : Acceptez-vous ? (Type 'Accept' or 'Deny')\n"));
               return 1;
            }
         }
         write_client(client->sock, "Le joueur n'existe pas\n");
         return 1;
      }
      return 1;
   } 
   
   else if (strcmp(p, "Accept") == 0)
   {
      for (int i = 0; i < *nbParties; i++)
      {
         if (parties[i].accepted == 0 && strcmp(parties[i].client2->name,client->name) == 0)
         {
            parties[i].accepted = 1;
            write_client(parties[i].client1->sock, "Défi accepté\n");
            return 1;
         }
      }
   }

   else if (strcmp(p, "Deny") == 0)
   {
      for (int i = 0; i < *nbParties; i++)
      {
         if (parties[i].accepted == 0 && strcmp(parties[i].client2->name,client->name) == 0)
         {
            write_client(parties[i].client1->sock, "Défi refusé\n");
            return 1;
         }
      }
   }
   
   else 
   {
      printf("Client %s : %s\n", client->name, buffer);
      send_message_to_all_clients(clients, *client, actual, buffer, 0);
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
               Client client = clients[i];
               int c = read_client(clients[i].sock, buffer);

               /* client disconnected */
               if (c == 0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
               }
               else
               {
                  command(parties, clients, actual, &nbParties, &client, buffer);
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

static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void remove_partie(Partie *parties, int to_remove, int *nbParties)
{
   /* we remove the client in the array */
   memmove(parties + to_remove, parties + to_remove + 1, (*nbParties - to_remove - 1) * sizeof(Partie));
   /* number client - 1 */
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

// {
//    char message[BUF_SIZE] = "";
//    for (int i = 0; i < TAILLE_PLATEAU; i++)
//    {
//       char case_plateau[TAILLE_PLATEAU];
//       sprintf(case_plateau, "%d ", plateau[i]);
//       strncat(message, case_plateau, sizeof message - strlen(message) - 1);
//    }
//    if(from_server == 0)
//    {
//       strncpy(message, sender.name, BUF_SIZE - 1);
//       strncat(message, " : ", sizeof message - strlen(message) - 1);
//    }
//    strncat(message, buffer, sizeof message - strlen(message) - 1);
//    write_client(client1.sock, message);
// }