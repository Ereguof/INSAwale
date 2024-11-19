#include <stdio.h>

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

typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

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

static void afficher_plateau(int plateau[], Client *client1, Client *client2, int num_joueur_appelant);
static int cote_adverse_vide(int plateau[], Client *client);
static int coup_valide(int plateau[], Client *client, int case_joueur);
static int coup_suivant(int plateau[], Client *client, int case_joueur);
static int fin_de_partie(Client *client1, Client *client2);

/**
 * @brief Initializes the server.
 */
static void init(void);

/**
 * @brief Cleans up the server before exiting.
 */
static void end(void);

/**
 * @brief Main application loop.
 */
static void app(void);

/**
 * @brief Initializes the server connection.
 * 
 * @return int Socket descriptor for the server connection.
 */
static int init_connection(void);

/**
 * @brief Ends the server connection.
 * 
 * @param sock Socket descriptor for the server connection.
 */
static void end_connection(int sock);

/**
 * @brief Reads data from a client socket.
 * 
 * @param sock Client socket descriptor.
 * @param buffer Buffer to store the read data.
 * @return int Number of bytes read.
 */
static int read_client(SOCKET sock, char *buffer);

/**
 * @brief Writes data to a client socket.
 * 
 * @param sock Client socket descriptor.
 * @param buffer Buffer containing the data to write.
 */
static void write_client(SOCKET sock, const char *buffer);

/**
 * @brief Sends a message to all connected clients.
 * 
 * @param clients Array of clients.
 * @param client The client sending the message.
 * @param actual Number of currently connected clients.
 * @param buffer Message to send.
 * @param from_server Flag indicating if the message is from the server.
 */
static void send_message_to_all_clients(Client *clients, Client client, int actual, const char *buffer, char from_server);

/**
 * @brief Removes a spectator from the list of spectators.
 * 
 * @param spectateurs Array of spectators.
 * @param nbSpectateurs Pointer to the number of spectators.
 * @param client The client to remove.
 */
static void remove_spectator(Client *spectateurs, int* nbSpectateurs, Client *client);

/**
 * @brief Removes a client from the list of clients.
 * 
 * @param clients Array of clients.
 * @param to_remove Index of the client to remove.
 * @param actual Pointer to the number of currently connected clients.
 * @param nbParties Pointer to the number of active games.
 * @param parties Array of active games.
 */
static void remove_client(Client *clients, int to_remove, int *actual, int *nbParties, Partie *parties);

/**
 * @brief Removes a game from the list of active games.
 * 
 * @param parties Array of active games.
 * @param to_remove Index of the game to remove.
 * @param nbParties Pointer to the number of active games.
 */
static void remove_game(Partie *parties, int to_remove, int *nbParties);

/**
 * @brief Clears the list of clients.
 * 
 * @param clients Array of clients.
 * @param actual Number of currently connected clients.
 */
static void clear_clients(Client *clients, int actual);

/**
 * @brief Processes a command from a client.
 * 
 * @param parties Array of active games.
 * @param clients Array of clients.
 * @param actual Number of currently connected clients.
 * @param nbParties Pointer to the number of active games.
 * @param client The client sending the command.
 * @param buffer Command buffer.
 * @return int Status code.
 */
static int command(Partie parties[MAX_PARTIES], Client clients[MAX_CLIENTS], int actual, int *nbParties, Client *client, char *buffer);

/**
 * @brief Checks if a client is spectating a game.
 * 
 * @param client The client to check.
 * @param parties Array of active games.
 * @param nbParties Number of active games.
 * @return int 1 if the client is spectating, 0 otherwise.
 */
static int inSpectate(Client *client, Partie parties[MAX_PARTIES], int nbParties);

/**
 * @brief Initializes the game board.
 * 
 * @param plateau Game board array.
 * @return int Status code.
 */
static int initBoard(int plateau[TAILLE_PLATEAU]);

/**
 * @brief Sends the game board to a client.
 * 
 * @param sock Client socket descriptor.
 * @param plateau Game board array.
 * @return int Status code.
 */
static int sendBoard(SOCKET sock, int plateau[TAILLE_PLATEAU]);

/**
 * @brief Sends the game score to a client.
 * 
 * @param sock Client socket descriptor.
 * @param partie Game data.
 * @return int Status code.
 */
static int sendScore(SOCKET sock, Partie *partie);

/**
 * @brief Deserializes an integer array from a buffer.
 * 
 * @param buffer Buffer containing the serialized data.
 * @param array Array to store the deserialized data.
 * @param size Size of the array.
 * @return int Status code.
 */
static int deserializeIntArray(char* buffer, int* array, int size);

/**
 * @brief Displays the game state.
 * 
 * @param partie Game data.
 * @return int Status code.
 */
static int showGame(Partie *partie);

/**
 * @brief Displays the list of active games.
 * 
 * @param parties Array of active games.
 * @param nbParties Number of active games.
 * @return int Status code.
 */
static int showGames(Partie *parties, int nbParties);


#endif /* guard */