/* Copied from git repo Practica_18. 
 * TODO: change camelCase to backspace (_) as C nd Python are more according to it
 * 
 */
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <errno.h>

#include <string.h>
#include <unistd.h>

#define PORT 9003
#define SIZEBUFF 1024

int main() {
    /*Create socket */
    int networkSocket, n;
    unsigned int length;
    struct sockaddr_in serverAdress;
    char serverResponse[SIZEBUFF];
    char *hello = "Hello from client";
    networkSocket = socket(AF_INET, SOCK_DGRAM, 0); /*Retorna el socket propi de l'aplicació ~*/

    /*Specify an address for the socket */
    serverAdress.sin_family = AF_INET;
    serverAdress.sin_port = htons(PORT); 
        /* 9002 és el port, htons ho converteix en un valor      
        addequat per a ser emmagatzemat a port */
    serverAdress.sin_addr.s_addr = INADDR_ANY; 
        /* Especifica l'adreça del computador a connectar.
        L'adressa es 0.0.0.0 (CONST de lib*), o sigui, alguna connexiód a la xarxa local */

    sendto(networkSocket, (const char *) hello, strlen(hello), MSG_CONFIRM, 
                (const struct sockaddr *) &serverAdress, sizeof(serverAdress));
    printf("Hello message sent:\n");

    n = recvfrom(networkSocket, (void *) serverResponse, SIZEBUFF, MSG_WAITALL,
             (struct sockaddr *) &serverAdress, &length);

    serverResponse[n] = '\0';

    /* print out solution and close connection */
    printf("The server sent the data: %s\n", serverResponse);
    close(networkSocket);

    return 0;

}
