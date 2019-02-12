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

#include <unistd.h>

#define PORT 9002
#define SIZEBUFF 150

int main() {
    /*Create socket */
    int networkSocket, len;
    struct sockaddr_in serverAdress;
    int connectionStatus;
    char server_response[SIZEBUFF];
    unsigned char client_response[SIZEBUFF] = "ABCD";
    networkSocket = socket(AF_INET, SOCK_STREAM, 0); 
        /*Retorna el socket propi de l'aplicació ~*/
    

    /*Specify an address for the socket */
    serverAdress.sin_family = AF_INET;
    serverAdress.sin_port = htons(PORT); 
        /* 9002 és el port, htons ho converteix en un valor      
        addequat per a ser emmagatzemat a port */
    serverAdress.sin_addr.s_addr = INADDR_ANY; 
        /* Especifica l'adreça del computador a connectar.
        L'adressa es 0.0.0.0 (CONST de lib*), o sigui, el propi ordinador */

    /* Connecta amb el servidor. Si no es dur a terme retorna -1 */
    connectionStatus = connect(networkSocket, (struct sockaddr *) &serverAdress, 
                             sizeof(serverAdress));

    /* Mira si hi ha un error en la connexio*/
    if (connectionStatus == -1) {
        perror("There was an error connecting to the server\n");
        exit(-1);
    }

    /* recieve data from the server*/
    len = recv(networkSocket, &server_response, sizeof(server_response), 0);
    printf("%i\n", len);
    server_response[len] = '\0';
    
    /* print out solution and close connection */
    printf("The server sent the data: %s\n", server_response);
    client_response[7] = 0x20;
    send(networkSocket, client_response, SIZEBUFF, 0);
    close(networkSocket);

    return 0;

}
