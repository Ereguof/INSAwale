#define TAILLE_PLATEAU 12
#define NB_GRAINES 48
#define NB_GRAINES_WIN 25
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h>  /* gethostbyname */
#include <stdio.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

typedef struct
{
    // SOCKET sock;
    // char name[BUF_SIZE];
    int numJoueur;
    int nbGraines;
} Client;

void afficher_plateau(int plateau[], Client *client1, Client *client2)
{
    printf("Plateau : \n");
    for (int i = TAILLE_PLATEAU - 1; i > TAILLE_PLATEAU / 2 - 1; i--)
    {
        printf("%d ", plateau[i]);
    }
    printf("\n");
    for (int i = 0; i < TAILLE_PLATEAU / 2; i++)
    {
        printf("%d ", plateau[i]);
    }
    printf("\n");
    printf("joueur 1 : %d\n", client1->nbGraines);
    printf("joueur 2 : %d\n", client2->nbGraines);
    printf("\n");
}

int cote_adverse_vide(int plateau[], Client *client)
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

int coup_valide(int plateau[], Client *client, int case_joueur)
{
    if (case_joueur < 1 || case_joueur > TAILLE_PLATEAU / 2)
    {
        printf("Case invalide : choisis le bon nombre\n");
        return 0;
    }
    if (client->numJoueur == 1)
    {
        if (cote_adverse_vide(plateau, client))
        {
            if (TAILLE_PLATEAU / 2 - case_joueur + 1 > plateau[case_joueur - 1])
            {
                printf("Case invalide : famine\n");
                return 0;
            }
        }
        if (plateau[case_joueur - 1] == 0)
        {
            printf("Case vide\n");
            return 0;
        }
    }
    else if (client->numJoueur == 2)
    {
        if (cote_adverse_vide(plateau, client))
        {
            if (TAILLE_PLATEAU / 2 - case_joueur + 1 > plateau[case_joueur - 1 + TAILLE_PLATEAU / 2])
            {
                printf("Case invalide : famine\n");
                return 0;
            }
        }
        if (plateau[case_joueur + 5] == 0)
        {
            printf("Case vide\n");
            return 0;
        }
    }
    return 1;
}

int coup_suivant(int plateau[], Client *client, int case_joueur)
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
            i++;
        }
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

    if (cote_adverse_vide(plateau, client))
    {
        for (int i = 0; i < TAILLE_PLATEAU; i++)
        {
            plateau[i] = savePlateau[i];
            client->nbGraines = saveGraines;
        }
    }
    return i;
}

void init_game(Client *client1, Client *client2, int plateau[])
{
    for (int i = 0; i < TAILLE_PLATEAU; i++)
    {
        plateau[i] = NB_GRAINES / TAILLE_PLATEAU;
    }
    client1->nbGraines = 0;
    client2->nbGraines = 0;
    client1->numJoueur = 1;
    client2->numJoueur = 2;
}

int fin_de_partie(Client *client1, Client *client2)
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

int main(int argc, char **argv)
{
    Client client1;
    Client client2;
    int plateau[TAILLE_PLATEAU];
    init_game(&client1, &client2, plateau);

    int client_actuel = 1;
    while (1)
    {
        int coup;
        printf("Joueur %d, choisissez une case\n", client_actuel);
        scanf("%d", &coup);
        if (client_actuel == 1)
        {
            if (coup_valide(plateau, &client1, coup))
            {
                coup_suivant(plateau, &client1, coup);
                afficher_plateau(plateau, &client1, &client2);
            }
            else
            {
                continue;
            }
        }
        else if (client_actuel == 2)
        {
            if (coup_valide(plateau, &client2, coup))
            {
                coup_suivant(plateau, &client2, coup);
                afficher_plateau(plateau, &client1, &client2);
            }
            else
            {
                continue;
            }
        }
        if (fin_de_partie(&client1, &client2))
        {
            printf("Fin de partie\n");
            break;
        }
        client_actuel = (client_actuel % 2) + 1;
    }

    return 0;
}