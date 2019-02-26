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
#include <netdb.h>

/* All constants */
#define CONFIG_SIZE 20
#define LINE_SIZE 35

/* All enums */
enum config {name, mac, server, port};
enum states {DISCONNECTED, WAIT_REG, REGISTERED,
             ALIVE};
enum error_package{CORRECT, RANDOM, MAC, NAME};
enum pac {REGISTER_REQ=0x00, REGISTER_ACK=0x01,
          REGISTER_NACK=0x02, REGISTER_REJ=0x03,
          ERROR=0x09,
          
          ALIVE_INF=0x10, ALIVE_ACK=0x11,
          ALIVE_NACK=0x12, ALIVE_REJ=0x13,
          
          SEND_FILE=0x20, SEND_ACK=0x21,
          SEND_NACK=0x22, SEND_REJ=0x23,
          SEND_DATA=0x24, SEND_END=0x25,
          
          GET_FILE=0x30, GET_ACK=0x31,
          GET_NACK=0x32, GET_REJ=0x33,
          GET_DATA=0x34, GET_END=0x35};

enum states {DISCONNECTED, WAIT_REG, REGISTERED,
             ALIVE};
            
enum time {N=3, T=2, M=4, P=8, S=5, Q=3};
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
    enum states state;
    long tcp_port;
    
} connection;

typedef struct udp_connect {
    int socket;
    struct sockaddr_in address;
} udp_connect;

typedef struct udp_package {
    unsigned char type;
    char name[7];
    char mac[13];
    char random[7];
    char data[50];
} udp_package;


/* All functions declarated */
Arg argparser(int argc, char **argv);
void debu(char *text, int debug);
connection udp_sock(char config[CONFIG_SIZE], int debug);
udp_connect connect(int server, int port, int debug);
int udp_recv(connection connection, udp_package *package);
void udp_send(connection connection, unsigned char type, 
              char data[50]);
void register_fase(connection connection, int debug);
int time(int iter);
int udp_package_checker(udp_package package, connection connection);
void quit(connection connection); /* TODO*/

int main(int argc, char **argv) {
    Arg arg;
    connection conn;
    arg = argparser(argc, argv);
    conn = udp_sock(arg.config, arg.debug);
    register_fase(conn, arg.debug);
    /* Begin concurrent staying_alive and 
     * CLI*/
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

/* TODO: read is not safe. Scanf is not safe. I'm quite sure that
 * server ip reading doesn't work not even near that what I would 
 * expect. Maybe %i.%i.%i.%i will help? It's necessary tho? ask 
 * professor maybe */
connection udp_sock(char config[CONFIG_SIZE], int debug) {
    connection conn;
    FILE *file_config;
    char *word;
    int server, port;
    char *line;


    line = (char *)malloc(LINE_SIZE * sizeof(char));
    if( line == NULL)
    {
        perror("Unable to allocate buffer");
        exit(1);
    }
    file_config = fopen(config, 'r'); /* TODO: Error checker
    * TODO:  Below line could fail if get line fails*/
    while(getline(&line, sizeof(line), file_config)) {
        word = strtok(line, " ");
        if(strcmp(word, "Nom") == 0){
            strcpy(conn.nom, strtok(NULL, " "));
        } else if(strcmp(word, "MAC") == 0) {
            strcpy(conn.nom, strtok(NULL, " "));
        } else if(strcmp(word, "Server") == 0) {
            word = (strtok(NULL, " "));
            if (strcmp(word, 'localhost') == 0){
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
    if(fclose(file_config) != 0) {
        debu("Error closing the file\n", debug);
    }
    conn.udp_connect = connect(server, port, debug);
    conn.state = DISCONNECTED;
    return conn;
}

/* TODO maybe it needs error checker*/
udp_connect connect(int server, int port, int debug){
    udp_connect connexion;
    connexion.socket = socket(AF_INET, SOCK_DGRAM, 0); /*Retorna el socket propi de l'aplicació ~*/
    if(connexion.socket < 0) {
        debu("Error creating socket\n", debug);
        exit(1);
    }
    memset(&connexion.address, 0, sizeof(connexion.address));
    /*Specify an address for the socket */
    connexion.address.sin_family = AF_INET;
    connexion.address.sin_port = htons(port);
    connexion.address.sin_addr.s_addr = htons(server);
    return connexion;
}

void udp_send(connection connection, unsigned char type, 
              char data[50]) {
    int i;
    udp_package package;
    package.type = type;
    strcpy(package.name, connection.nom);
    strcpy(package.mac, connection.mac);
    if (connection.state == DISCONNECTED 
       || connection.state == WAIT_REG) {
           memset(package.random, 0, sizeof(package.random));
    } else {
        /* sizeof(char) == 1, so sizeof(char[size]) == size )*/
        for(i = 0; i < sizeof(package.random); i++) {
            package.random[i] = connection.random[i];
        }
    }
    strcpy(package.data, data);
    sendto(connection.udp_connect.socket, (void *) &package, 
           sizof(package), 0, 
           (const struct sockaddr *) &connection.udp_connect.address,
           sizeof(connection.udp_connect.address));
}

/* Doesnt check errors */
int udp_recv(connection connection, udp_package *package) {
    return recvfrom(connection.udp_connect.socket, (void *) &package, 
           sizof(package), 0, 
           (const struct sockaddr *) &connection.udp_connect.address,
           sizeof(connection.udp_connect.address));
}

/* TODO: add checker for MAC addres and name.
 *       add debugger, for god's sake!!*/
void register_fase(connection connection, int debug) {
    /* TODO: */
    char data[50];
    udp_package package;
    int bytes_readed;
    int q, p; /* Iter q and p respectively*/
    memset((void *) data, '\0', sizeof(data));
    package.type = REGISTER_NACK; 
        /*initialize to some value so it doesnt breaks*/
    q = 0;
    while(q < Q) {
        p = 0;
        bytes_readed = 0;
        connection.state = WAIT_REG;
        while(p < P && bytes_readed == 0) {
            udp_send(connection, REGISTER_REQ, data);
            alarm(T*time(p));
            bytes_readed = udp_recv(connection, &package);
            p++;
        }
        if (package.type == REGISTER_REJ) {
            printf("Register fase rejected from server"
                   ", code error: %s\n", package.data);
            quit(connection);
        } else if(package.type == REGISTER_ACK) {
            connection.state = REGISTERED;
            strcpy(connection.random, package.random);
            sscanf(package.data, "%ld", connection.tcp_port);
        } else { 
            /* Either if its ignored enough times or it 
             * is NACK will go down here*/
            q++;
        }
    }
    udp_send(connection, REGISTER_REQ, data);
}

/* Some of this may be needed to be CONSTANTS in
 * Or a enum type having all of them. "Imported"
 * from ..\tmp\time.c */
int time(int i) {
    if(i < N) {
        return 1;
    } else if(i-N+2 < M) {
        return (i-N+2);
    } else {
        return M;
    }
}


/* Returns an internal error code. 
 * 0 If its all correct
 * 1 if Random number fails
 * 2 if MAC test fails
 * 3 if name test fails
 * All of them are in enum error_package, and
 * it should be used when managing this function
 * 
 * Type package test is left to the protocol management,
 * as it would make this function unnecessary large*/
int udp_package_checker(udp_package package, connection connection) {
    if(strcmp(package.name, connection.nom)){
        return NAME;
    } else if(strcmp(package.mac, connection.mac)) {
        return MAC;
    } else if(strcmp(package.random, connection.random)) {
        return RANDOM;
    } else {
        return CORRECT;
    }
}



