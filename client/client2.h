#ifndef CLIENT_H
#define CLIENT_H

#ifdef WIN32

#include <winsock2.h>

#elif defined (linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
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

//
#define CRLF "\r\n"
#define PORT 1977
#define BUF_SIZE 1024

/**
 * @brief Initializes the application.
 */
static void init(void);

/**
 * @brief Cleans up resources before the application ends.
 */
static void end(void);

/**
 * @brief Main application logic.
 * 
 * @param address The server address to connect to.
 * @param name The name of the client.
 */
static void app(const char *address, const char *name);

/**
 * @brief Initializes a connection to the server.
 * 
 * @param address The server address to connect to.
 * @return int The socket file descriptor, or -1 on error.
 */
static int init_connection(const char *address);

/**
 * @brief Ends the connection to the server.
 * 
 * @param sock The socket file descriptor.
 */
static void end_connection(int sock);

/**
 * @brief Reads data from the server.
 * 
 * @param sock The socket file descriptor.
 * @param buffer The buffer to store the data.
 * @return int The number of bytes read, or -1 on error.
 */
static int read_server(SOCKET sock, char *buffer);

/**
 * @brief Writes data to the server.
 * 
 * @param sock The socket file descriptor.
 * @param buffer The data to send.
 */
static void write_server(SOCKET sock, const char *buffer);

#endif /* guard */
