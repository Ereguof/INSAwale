#ifndef CLIENT_H
#define CLIENT_H

typedef struct Partie Partie;

#include "server2.h"

struct Client
{
   SOCKET sock;
   char name[BUF_SIZE];
   char bio[BUF_SIZE/2];
   int numJoueur;
   int nbGraines;
   Partie * partie;
};

#endif /* guard */