#ifndef CLIENT_H
#define CLIENT_H

#include "server2.h"

typedef struct
{
   SOCKET sock;
   char name[BUF_SIZE];
   int numJoueur;
   int nbGraines;
}Client;

#endif /* guard */
