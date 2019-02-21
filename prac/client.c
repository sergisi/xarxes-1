/* Includes done to the project */
#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

/* All constants */
#define CONFIG_SIZE 20

/* All enums */
enum config {name, mac, server, port};
/* All structs used in the program */
/* It will contain all arguments passed trough the call of the 
 * the function */
typedef struct arg {
    char config[CONFIG_SIZE];
    char file[CONFIG_SIZE];
    int debug;
} Arg;

/* All information needed for a all connections:
 * nom, MAC address, nombre aleatori i els dos sockets per
 * a la connexio de udp */
typedef struct connection {
    char nom[7];
    char mac[13];
    char random[7];
    udp_connect udp_connect;
} Conn;

typedef struct udp_connect {
    int socket;
    struct sockaddr_in address;
} udp_connect;

/* All functions declarated */
Arg argparser(int argc, char **argv);
void debu(char *text, int debug);
Conn udp_sock(char config[CONFIG_SIZE], int debug);
udp_connect connect(int server, int port);

int main(int argc, char **argv) {
    Arg arg;
    Conn conn;
    arg = argparser(argc, argv);
    conn = udp_sock(arg.config, arg.debug);
    return 0;
}

Arg argparser(int argc, char **argv){
    int c; //temporal integer for switch.
    Arg arg;
    
    arg.debug = 0;
    sprintf(arg.config, "client.cfg");
    sprintf(arg.file, "boot.cfg");
    while((c = getopt(argc, argv, "dc:f:")) != -1) {
        switch (c)
        {
            case 'd':
                arg.debug = 1;
                break;
            case 'c':
                sprintf(arg.config, "%s", optarg);
                break;
            case 'f':
                sprintf(arg.file, "%s", optarg);
            default:
                break;
        }
    }
    if (optind < argc) {
        printf("Too many arguments, they will be omitted\n");
    }

    return arg;
}

void debu(char *text, int debug){
    if (debug != 0) {
        printf("%s", text);
    }
}

Conn udp_sock(char config[CONFIG_SIZE], int debug) {
    Conn conn;
    int fd_config;
    char line[150];
    char *word;
    int server, port;
    fd_config = fopen(config, 'r');
    read(fd_config, line, sizeof(line)); //It should read all the doc
    word = strtok(line, " ");
    while(word != NULL) {
        if(strcmp(word, "Nom") == 0){
            strcpy(conn.nom, strtok(NULL, " "));
        } else if(strcmp(word, "MAC") == 0) {
            strcpy(conn.nom, strtok(NULL, " "));
        } else if(strcmp(word, "Server") == 0) {
            word = strtok(NULL, " ");
            if(strcmp(word, "localhost") == 0) {
                server = INADDR_LOOPBACK;
            } else {
                sscanf(word, "%i", &server);
            }
        } else if(strcmp(word, "Server-port") == 0) {
            sscanf(strtok(NULL, " "), "%i", &port);
        } else {
            debu("Not an accepted parameter\n", debug);
        }
        word = strtok(NULL, " ");
    }
    close(fd_config);
    conn.udp_connect = connect(server, port);
    return conn;
}

udp_connect connect(int server, int port){
    udp_connect connexion;
    connexion.socket = socket(AF_INET, SOCK_DGRAM, 0); /*Retorna el socket propi de l'aplicació ~*/

    /*Specify an address for the socket */
    connexion.address.sin_family = AF_INET;
    connexion.address.sin_port = htons(port);
    connexion.address.sin_addr.s_addr = htons(server);
    return connexion;
}



