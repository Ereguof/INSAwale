#ifdef WIN32

#include <winsock2.h>

#elif defined(linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h>  /* gethostbyname */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

#ifndef SERVER_H
#define SERVER_H

typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#define CRLF "\r\n"
#define PORT 1977
#define MAX_CLIENTS 100
#define MAX_PARTIES 50
#define BUF_SIZE 1024

#define TAILLE_PLATEAU 12
#define NB_GRAINES 48
#define NB_GRAINES_WIN 25

// Forward declaration of Client struct
typedef struct Client Client;

#include "client2.h"

struct Partie
{
    int accepted;
    int tour;
    int nbSpectateurs;
    int plateau[TAILLE_PLATEAU];
    Client *client1;
    Client *client2;
    Client spectateurs[MAX_CLIENTS];
};



static void init(void);
static void end(void);
static void app(void);
static int init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void send_message_to_all_clients(Client *clients, Client client, int actual, const char *buffer, char from_server);
static void remove_spectateur(Client *spectateurs, int* nbSpectateurs, Client *client);
static void remove_client(Client *clients, int to_remove, int *actual, int *nbParties, Partie *parties);
static void remove_partie(Partie *parties, int to_remove, int *nbParties);
static void clear_clients(Client *clients, int actual);

#endif /* guard */