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

struct Udp {
    unsigned char type;
    char name[7];
    char mac[13];
    char random[7];
    char data[50];
};


int main() {
    /*Create socket */
    int networkSocket, n;
    unsigned int length;
    struct sockaddr_in serverAdress;
    struct Udp resp, recv;
    resp.type = 0x20;
    strcpy(resp.name,  "i-700");
    strcpy(resp.mac, "89F107457A36");
    strcpy(resp.random, "0");
    strcpy(resp.data, "Just some struct passing program");
    networkSocket = socket(AF_INET, SOCK_DGRAM, 0); /*Retorna el socket propi de l'aplicació ~*/

    /*Specify an address for the socket */
    serverAdress.sin_family = AF_INET;
    serverAdress.sin_port = htons(PORT); 
        /* 9002 és el port, htons ho converteix en un valor      
        addequat per a ser emmagatzemat a port */
    serverAdress.sin_addr.s_addr = INADDR_ANY; 
        /* Especifica l'adreça del computador a connectar.
        L'adressa es 0.0.0.0 (CONST de lib*), o sigui, alguna connexiód a la xarxa local */

    sendto(networkSocket, &resp, sizeof(resp), MSG_CONFIRM, 
                (const struct sockaddr *) &serverAdress, sizeof(serverAdress));
    printf("Hello message sent:\n");

    n = recvfrom(networkSocket, (void *) &recv, sizeof(recv), MSG_WAITALL,
             (struct sockaddr *) &serverAdress, &length);

    /* print out solution and close connection */
    printf("The server sent the data: %s\n", recv.data);
    close(networkSocket);

    return 0;

}
