/* Includes done to the project */
#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>

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
            
enum time {N=3, T=2, M=4, P=8, S=5, Q=3, R=3, U=3, W=4};
enum command {SEND, RECV, QUIT};
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

typedef struct tcp_package {
    unsigned char type;
    char name[7];
    char mac[13];
    char random[7];
    char data[150];
} tcp_package;

short cld_alive;

/* All functions declarated */
Arg argparser(int argc, char **argv);
void debu(char *text, int debug);
connection udp_sock(char config[CONFIG_SIZE], int debug);
udp_connect udp_connection(int server, int port, int debug);
int udp_recv(connection connection, udp_package *package);
void udp_send(connection connection, unsigned char type, 
              char data[50]);
void register_fase(connection connection, int debug);
int time(int iter);
int udp_package_checker(udp_package package, connection connection);
void alive_fase(connection connection, int debug);
int tcp_connection(connection connection, int debug);

/* TODO: change alarm to select when possible, study how the timeout
 * works in that function */
int main(int argc, char **argv) {
    Arg arg;
    connection conn;
    int pd;
    arg = argparser(argc, argv);
    conn = udp_sock(arg.config, arg.debug);
    while(1) {
        register_fase(conn, arg.debug);
        /* Begin concurrent staying_alive and 
        * CLI, main should handle concurrency, 
        * functions have been made for staying_alive
        * and cli management. */
        cld_alive = 0;
        pd = fork();
        if (pd == -1) {
            perror("An error has ocurred during fork");
        } else if(pd == 0) {
            signal(SIGUSR1, sigusr1handler);
            cli(conn, arg);
        } else {
            signal(SIGUSR1, sigusr1handler);
            alive_fase(conn, arg.debug);
            kill(SIGUSR1, pd);
            wait();
        }
    }
    return 1;
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

/* TODO: I'm quite sure that
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
        word = strtok(line, " \n");
        if(strcmp(word, "Nom") == 0){
            strcpy(conn.nom, strtok(NULL, " \n"));
        } else if(strcmp(word, "MAC") == 0) {
            strcpy(conn.mac, strtok(NULL, " \n"));
        } else if(strcmp(word, "Server") == 0) {
            server = gethostbyname((strtok(NULL, " \n")));
        } else if(strcmp(word, "Server-port") == 0) {
            sscanf(strtok(NULL, " \n"), "%i", &port);
        } else {
            debu("Not an accepted parameter\n", debug);
        }
        word = strtok(NULL, " ");
    }
    free(line);
    if(fclose(file_config) != 0) {
        debu("Error closing the file\n", debug);
    }
    conn.udp_connect = udp_connection(server, port, debug);
    conn.state = DISCONNECTED;
    return conn;
}

/* TODO maybe it needs error checker*/
udp_connect udp_connection(int server, int port, int debug){
    udp_connect connexion;
    connexion.socket = socket(AF_INET, SOCK_DGRAM, 0); /*Retorna el socket propi de l'aplicació ~*/
    if(connexion.socket < 0) {
        debu("Error creating udp socket\n", debug);
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
    char data[50];
    fd_set rfds;
    udp_package package;
    short boolean;
    int q, p; /* Iter q and p respectively*/
    struct timeval tv;
    memset((void *) data, '\0', sizeof(data));
    package.type = REGISTER_NACK; 
        /*initialize to some value so it doesnt breaks*/
    q = 0;
    while(q < Q) {
        p = 0;
        boolean = 0;

        connection.state = WAIT_REG;
        while(p < P && boolean == 0) {
            udp_send(connection, REGISTER_REQ, data);
            tv.tv_sec = T*time(p);
            FD_ZERO(&rfds);
            FD_SET(connection.udp_connect.socket, &rfds);
            select(connection.udp_connect.socket, &rfds
                   , NULL, NULL, &tv);
            if(FD_ISSET(connection.udp_connect.socket, &rfds)){
                udp_recv(connection, &package);
                boolean = 1;
            } else {
                p++;
            }
        }
        if (package.type == REGISTER_REJ) {
            printf("Register fase rejected from server"
                   ", code error: %s\n", package.data);
            close(connection.udp_connect.socket);
            exit(-1);
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

/* Add sighandler for end when child gets quit command */
void alive_fase(connection connection, int debug) {
    int alive;
    fd_set rfds;
    struct timeval tv;
    udp_package package;
    char data[50];
    memset((void *) data, '\0', sizeof(data));
    alive = 0;
    tv.tv_sec = R;
    udp_send(connection, ALIVE_INF, data);
    while(alive < 0 && cld_alive == 0) { /* Change number*/
        alive++;
        FD_ZERO(&rfds);
        FD_SET(connection.udp_connect.socket, &rfds);
        select(connection.udp_connect.socket, &rfds
                   , NULL, NULL, &tv);
        if (FD_ISSET(connection.udp_connect.socket, &rfds)) {
            udp_recv(connection, &package);
                if(udp_package_checker(package, connection) != 0) {
                connection.state = ALIVE;
                if (package.type == ALIVE_ACK) {
                    alive--;
                } else if (package.type == ALIVE_REJ) {
                    to_register_fase(connection, debug);
                }
            }
        } else {
            tv.tv_sec = R;
            udp_send(connection, ALIVE_INF, data);
        }
    }
    if(cld_alive == 0) {
        debu("Packages alive lost, state pass to disconnected\n", debug);
    } else {
        printf("Shutting down\n");
        wait();
        exit(0);
    }
}

void sigusr1handler(int status) {
    cld_alive = 1;
}

/* TODO: add signal handler so parent can send signal
 * if anything occurs. Signal should break the while 
 * so data doesn't break corrupted. With select may be
 * unnecessary. */
void cli(connection connection, Arg arg) {
    while(cld_alive == 0) {
        switch(getcommand()) {
            case SEND:
                send_prot(connection, arg);
                break;
            case RECV:
                get_prot(connection, arg);
                break;
            case QUIT:
                /* TODO: handle first father/son end  then quit. With
                 * select may be unnecessary */
                kill(getppid(), SIGUSR1);
                exit(0);
                break; /* Unreachable line, as quit exits*/
            default:
                debu("Not a client build function, to get use of it"
                     "built-in commands ara: send-conf," 
                     "recv-conf and quit\n", arg.debug);
        }
    }
    debu("CLI is shutting down\n", arg.debug);
    exit(0);
}

/* TODO: I don't like it, redo*/
int getcommand() {
    char buffer[256];
    int i;
    char *word;
    scanf("%s", buffer);
    word = strtok(buffer, " ");
    if(strcmp(word, "send-conf") == 0) {
        return SEND;
    } else if(strcmp(word, "send-conf") == 0) {
        return RECV;
    } else if(strcmp(word, "send-conf") == 0) {
        return QUIT;
    } else {
        return -1;
    }
}

int tcp_connection(connection connection, int debug) {
    int tcp_socket;
    struct sockaddr_in server_address;
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(connection.tcp_port);
    server_address.sin_addr.s_addr = 
            connection.udp_connect.address.sin_addr.s_addr;
    

    if(connect(tcp_socket, (struct sockaddr *) &server_address, 
                             sizeof(server_address)) == -1) {
        debu("An error has occurred during the tcp connection, "
             "You must restart manually\n", debug);
    }
    return tcp_socket;
}

int tcp_send(connection connection, int sock, unsigned char type, 
             char data[150]){
    int i;
    tcp_package package;
    package.type = type;
    strcpy(package.name, connection.nom);
    strcpy(package.mac, connection.mac);
    for (i = 0; i < sizeof(package.random); i++) {
        package.random[i] = connection.random[i];
    }
    strcpy(package.data, data);
    send(sock, &package, sizeof(package), 0);
}

int tcp_package_checker(tcp_package package, connection connection) {
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

int tcp_recv(int socket, tcp_package *package) {
    return recv(socket, (void *) &package, sizeof(package), 0);
}

void send_prot(connection connection, Arg arg) {
    int socket;
    tcp_package package;
    struct timeval tv;
    fd_set rfds;
    char namecfg[7+4], data[150];
    strcpy(namecfg, connection.nom);
    data[0] = '\0';
    socket = tcp_connection(connection, arg.debug);
    tcp_send(connection, socket, SEND_FILE, data);
    tv.tv_sec = W;
    FD_ZERO(&rfds);
    FD_SET(connection.udp_connect.socket, &rfds);
    select(connection.udp_connect.socket, &rfds
            , NULL, NULL, &tv);
    if (FD_ISSET(socket, &rfds)) {
        tcp_recv(socket, &package);
        if(tcp_package_checker(package, connection) == 0) {
            if(package.type != SEND_ACK) {
                if(strcmp(package.data, strcat(namecfg, ".cfg"))) {
                    send_file(connection, socket, arg);
                } else {
                    printf("ERROR_SEND: Name camp was not according to this node\n");
                }
            } else {
                printf("ERROR_SEND: Package type was not GET_ACK: %x", package.type);
            }
        } else {
            printf("ERROR_SEND: Camps were not send accordingly\n");
        }

    } else {
        printf("ERROR_SEND: A trouble has occured during connexion with server."
               "to try another time, please put the command\n");
    }
    close(socket);
}


void send_file(connection connection, int socket, Arg arg){
    FILE *file;
    char line[150];
    file = fopen(arg.file, 'r');
    while(getline(&line, sizeof(line), file) > 0) {
        tcp_send(connection, socket, SEND_DATA, line);
        debu("SEND_INFO: sending data to server...\n", arg.debug);
    }
    debu("SEND_INFO: sending end to server\n", arg.debug);
    line[0]='\0'; /* so it sends a void string*/
    tcp_send(connection, socket, SEND_END, line);   
}


void recv_prot(connection connection, Arg arg) {
        int socket, n_bytes;
    tcp_package package;
    char namecfg[7+4], data[150];
    strcpy(namecfg, connection.nom);
    data[0] = '\0';
    socket = tcp_connection(connection, arg.debug);
    tcp_send(connection, socket, GET_FILE, data);
    alarm(W); /* TODO atm best solution seems, add sighandler*/
    n_bytes = tcp_recv(socket, &package);
    if (n_bytes != 0) {
        if(tcp_package_checker(package, connection) == 0) {
            if(package.type != GET_ACK) {
                if(strcmp(package.data, strcat(namecfg, ".cfg"))) {
                    get_file(connection, socket, arg);
                } else {
                    printf("ERROR_SEND: Name camp was not according to this node\n");
                }
            } else {
                printf("ERROR_SEND: Package type was not GET_ACK: %x",
                         package.type);
            }
        } else {
            printf("ERROR_SEND: Camps were not send accordingly\n");
        }

    } else {
        printf("ERROR_SEND: A trouble has occured during connexion with server."
               "to try another time, please put the command\n");
    }
    close(socket);
}


void get_file(connection connection, int socket, Arg arg) {
    FILE *file;
    char line[150];
    tcp_package package;
    file = fopen(arg.file, 'w');
    tcp_recv(socket, &package); 
    while(package.type != GET_END) {
        /* I don't know if the server sends or not sends \n*/
        fprintf(file, "%s\n", package.data); 
        debu("GET_INFO: sending data to server...\n", arg.debug);
        tcp_recv(socket, &package); 
    }
    debu("GET_INFO: sending end to server\n", arg.debug);
    line[0]='\0'; /* so it sends a void string*/
    tcp_send(connection, socket, SEND_END, line);   
}

