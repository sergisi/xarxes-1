/* Includes done to the project */
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <getopt.h>
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
#include <sys/wait.h>

/* Don't know if needed, as is POSIX 2001 and 2008*/

/* All constants */
#define CONFIG_SIZE 20
#define LINE_SIZE 50

/* All enums */
enum config {name, mac, server, port};
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
/* All structs used inudp_connect the program */
/* It will contain all arguments passed trough the call of the 
 * the function */
typedef struct arg {
    char config[CONFIG_SIZE];
    char file[CONFIG_SIZE];
    int debug;
} Arg;

typedef struct udp_connect {
    int socket;
    struct sockaddr_in address;
} udp_connect;

struct server {
    char nom[7];
    char mac[13];
};
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
    struct server server;
} connection;

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

/* All functions declarated */
Arg argparser(int argc, char **argv);
void debu(char *text, int debug, int level);
connection udp_sock(char config[CONFIG_SIZE], int debug);
udp_connect udp_connection(struct hostent* ent, int port, int debug);
int udp_recv(connection connection, udp_package *package);
void udp_send(connection connection, unsigned char type, 
              char data[50], int debug);
void register_fase(connection *connection, int debug);
int time(int iter);
int udp_package_checker(udp_package package, connection connection);
void alive_fase(connection connection, int debug, int pipes[2][2]);
int tcp_connection(connection connection, int debug);
void cli(connection connection, Arg arg, int pipes[2][2]);
void send_prot(connection connection, Arg arg);
void send_file(connection connection, int socket, Arg arg);
void get_prot(connection connection, Arg arg);
void get_file(connection connection, int socket, Arg arg);
int getcommand();
void debu_tcp_package(tcp_package package, int debug, int level);
void debu_udp_package(udp_package package, int debug, int level);
char* getsize(char *path);

/* TODO: change alarm to select when possible, study how the timeout
 * works in that function */
int main(int argc, char **argv) {
    Arg arg;
    connection conn;
    int pd;
    int pipes[2][2];

    arg = argparser(argc, argv);
    debu("Finished argparser\n", arg.debug, 1);
    conn = udp_sock(arg.config, arg.debug);
    while(1) {
        debu("Starting register fase\n", arg.debug, 1);
        register_fase(&conn, arg.debug);
        debu("Register fase finished\n", arg.debug, 1);
        /* Begin concurrent staying_alive and 
        * CLI, main should handle concurrency, 
        * functions have been made for staying_alive
        * and cli management. */
        if(pipe(pipes[0]) != 0 ) {
            printf("Error creating a pipe\n");
        }
        if(pipe(pipes[1]) != 0 ) {
            printf("Error creating a pipe\n");
        }
        pd = fork();
        if (pd == -1) {
            perror("An error has ocurred during fork");
        } else if(pd == 0) {
            close(pipes[0][0]);
            close(pipes[1][1]);
            debu("Starting CLI\n", arg.debug, 1);
            cli(conn, arg, pipes);
        } else {
            close(pipes[0][1]);
            close(pipes[1][0]);
            debu("Starting alive fase\n", arg.debug, 1);
            alive_fase(conn, arg.debug, pipes);
            debu("Finished alive fase\n", arg.debug, 1);
        }
    }
    return 1;
}

Arg argparser(int argc, char **argv){
    int c; /* temporal integer for switch. */
    Arg arg;
    
    arg.debug = 0;
    sprintf(arg.config, "client.cfg");
    sprintf(arg.file, "boot.cfg");
    while((c = getopt(argc, argv, "d:c:f:")) != -1) {
        switch (c)
        {
            case 'd':
                sscanf(optarg, "%i", &arg.debug);
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

void debu(char *text, int debug, int level){
    if (level <= debug) {
        printf("%s", text);
    }
}

void debu_udp_package(udp_package package, int debug, int level) {
        if (level <= debug) {
            printf("PACKAGE_INFO: the data in package is:\n"
                    "\ttype: %x\tname: %s\tmac: %s\talea: %s\tdata: %s\n",
                    package.type, package.name, package.mac, package.random,
                    package.data);
    }
}

void debu_tcp_package(tcp_package package, int debug, int level) {
        if (level <= debug) {
            printf("PACKAGE_INFO: the data in package is:\n"
                    "\ttype: %x\tname: %s\tmac: %s\talea: %s\tdata: %s\n",
                    package.type, package.name, package.mac, package.random,
                    package.data);
    }
}

/* TODO: I'm quite sure that
 * server ip reading doesn't work not even near that what I would 
 * expect. Maybe %i.%i.%i.%i will help? It's necessary tho? ask 
 * professor maybe */
connection udp_sock(char config[CONFIG_SIZE], int debug) {
    connection conn;
    FILE *file_config;
    char word[2][20];
    int port;
    char *line;
    struct hostent *ent;

    debu("INIT_UDP_SOCK_INFO: initialized variables\n", debug, 2);
    line = (char *) malloc(LINE_SIZE * sizeof(char));
    if( line == NULL)
    {
        perror("Unable to allocate buffer");
        exit(1);
    }

    file_config = fopen(config, "r"); /* TODO: Error checker
    * TODO:  Below line could fail if get line fails*/
    debu("INIT_UDP_SOCK_INFO: opened configuration file\n", debug, 9);
    while(fscanf(file_config, "%s %s", word[0], word[1]) != EOF) {
        debu("INIT_UDP_SOCK_INFO: read a line of config\n", debug, 9);
        if(strcmp(word[0], "Nom") == 0){
            strcpy(conn.nom, word[1]);
            debu("INIT_UDP_SOCK_INFO: name initialized\n", debug, 9);
        } else if(strcmp(word[0], "MAC") == 0) {
            strcpy(conn.mac, word[1]);
            debu("INIT_UDP_SOCK_INFO: mac initialized\n", debug, 9);
        } else if(strcmp(word[0], "Server") == 0) {
            ent = gethostbyname(word[1]);
            debu("INIT_UDP_SOCK_INFO: gethostbyname reached\n", debug, 9);
        } else if(strcmp(word[0], "Server-port") == 0) {
            sscanf(word[1], "%i", &port);
            debu("INIT_UDP_SOCK_INFO: port initialized\n", debug, 9);
        } else {
            debu("Not an accepted parameter\n", debug, 9);
        }
    }
    free(line);
    if(fclose(file_config) != 0) {
        debu("Error closing the file\n", debug, 1);
    }
    debu("INIT_UDP_SOCK_INFO: Starting udp_socket\n", debug, 1);
    conn.udp_connect = udp_connection(ent, port, debug);
    conn.state = DISCONNECTED;
    debu("STATE_INFO: DISCONNECTED\n", debug, 1);
    return conn;
}

/* TODO maybe it needs error checker*/
udp_connect udp_connection(struct hostent* ent, int port, int debug) {
    udp_connect connexion;
    connexion.socket = socket(AF_INET, SOCK_DGRAM, 0); /*Retorna el socket propi de l'aplicació ~*/
    if(connexion.socket < 0) {
        debu("Error creating udp socket\n", debug, 1);
        exit(1);
    }

	memset(&connexion.address, 0, sizeof(struct sockaddr_in));
	connexion.address.sin_family=AF_INET;
	connexion.address.sin_addr.s_addr=(((struct in_addr *)ent->h_addr_list[0])->s_addr);
	connexion.address.sin_port=htons(port);
    return connexion;
}

void udp_send(connection connection, unsigned char type, 
              char data[50], int debug) {
    udp_package package;
    package.type = type;
    strcpy(package.name, connection.nom);
    strcpy(package.mac, connection.mac);

    /* sizeof(char) == 1, so sizeof(char[size]) == size )*/
    strcpy(package.random, connection.random);

    strcpy(package.data, data);
    debu("UDP_SEND > ", debug, 9);
    debu_udp_package(package, debug, 9);
    sendto(connection.udp_connect.socket, (void *) &package, 
           sizeof(package), 0, 
           (const struct sockaddr *) &connection.udp_connect.address,
           sizeof(connection.udp_connect.address));
}

/* Doesnt check errors */
int udp_recv(connection connection, udp_package *package) {
    return recvfrom(connection.udp_connect.socket, (void *) package, 
           sizeof(*package), 0, 
           (struct sockaddr *) &connection.udp_connect.address,
           (socklen_t *) sizeof(connection.udp_connect.address));
}

/* TODO: add checker for MAC addres and name.
 *       add debugger, for god's sake!!*/
void register_fase(connection *connection, int debug) {
    char data[50];
    fd_set rfds;
    udp_package package;
    short boolean;
    int q, p; /* Iter q and p respectively*/
    struct timeval tv;
    memset((void *) data, '\0', sizeof(data));
    debu("REGISTER_INFO: initialized data\n", debug, 1);
    package.type = REGISTER_NACK;
    strcpy((*connection).random, "000000");
        /*initialize to some value so it doesnt breaks*/
    q = 0;
    boolean = 0;
    while(q < Q && boolean == 0) {
        debu("REGISTER_INFO: initialized process to register\n", debug, 5);
        p = 0;
        (*connection).state = WAIT_REG;
        while(p < P && boolean == 0) {
            debu("REGISTER_INFO: sending REGISTER_REQ\n", debug, 5);
            udp_send(*connection, REGISTER_REQ, data, debug);
            tv.tv_sec = T*time(p);
            tv.tv_usec = 0;
            FD_ZERO(&rfds);
            FD_SET((*connection).udp_connect.socket, &rfds);
            select((*connection).udp_connect.socket + 1, &rfds
                   , NULL, NULL, &tv);
            if(FD_ISSET((*connection).udp_connect.socket, &rfds)){
                udp_recv((*connection), &package);
                debu("REGISTER_INFO: recv package\n", debug, 5);
                if (package.type == REGISTER_REJ) {
                    printf("Register fase rejected from server"
                        ", code error: %s\n", package.data);
                    close((*connection).udp_connect.socket);
                    exit(-1);
                } else if(package.type == REGISTER_ACK) {
                    debu("REGISTER_INFO: registered successfully\n", debug, 1);
                    (*connection).state = REGISTERED;
                    strcpy((*connection).random, package.random);
                    strcpy((*connection).server.nom, package.name);
                    strcpy((*connection).server.mac, package.mac);
                    sscanf(package.data, "%ld", &(*connection).tcp_port);
                    boolean = 1;
                } else { 
                    debu("REGISTER_INFO: not an ACK package\n", debug, 2);
                    /* Either if its ignored enough times or it 
                    * is NACK will go down here*/
                    select(0, NULL, NULL, NULL, &tv); /* So it waits the rest */
                    p++;
                }
                debu_udp_package(package, debug, 5);
            } else {
                p++;
            }
        }
        if(boolean == 0) {
            sleep(S);
            q++;
        }
    }
    if((*connection).state != REGISTERED) {
        printf("Unnable to register to server\n");
        close((*connection).udp_connect.socket);
        exit(-1);
    }
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
    if(strcmp(package.name, connection.server.nom) != 0){
        return NAME;
    } else if(strcmp(package.mac, connection.server.mac) != 0) {
        return MAC;
    } else if(strcmp(package.random, connection.random) != 0) {
        return RANDOM;
    } else {
        return CORRECT;
    }
}

/* Add sighandler for end when child gets quit command */
void alive_fase(connection connection, int debug, int pipes[2][2]) {
    int alive;
    short boolean;
    fd_set rfds;
    struct timeval tv;
    udp_package package;
    char data[50];
    char child;
    memset((void *) data, '\0', sizeof(data));
    debu("ALIVE_INFO: started alive process\n", debug, 1);
    boolean = 0;
    tv.tv_sec = R;
    debu("Sending first alive\n", debug, 9);
    udp_send(connection, ALIVE_INF, data, debug);
    alive = 1; 
    /* TODO: ask professor what means 3 packages with no response 
     * added equal as it seems it wants this with -t 2 */
    while(alive <= U && boolean == 0) { /* Change number*/
        FD_ZERO(&rfds);
        FD_SET(pipes[0][0], &rfds);
        tv.tv_usec= 0;
        tv.tv_sec = R;
        select(pipes[0][0] + 1, &rfds
                   , NULL, NULL, &tv);
        if (FD_ISSET(pipes[0][0], &rfds)) { /* May be both are set */
            debu("PIPE_PRNT: Shutting down\n", debug, 5);
            close(connection.udp_connect.socket);
            close(pipes[0][0]);
            close(pipes[1][1]);
            if(wait(NULL) == -1) {
                perror("Wait failed");
                exit(-1);
            }
            exit(0);
        }
        tv.tv_usec= 0;
        tv.tv_sec = 0;
        FD_ZERO(&rfds);
        FD_SET(connection.udp_connect.socket, &rfds);
        select(connection.udp_connect.socket + 1, &rfds
                   , NULL, NULL, &tv);
        if (FD_ISSET(connection.udp_connect.socket, &rfds)) {
            udp_recv(connection, &package);
            debu_udp_package(package, debug, 9);
            if(udp_package_checker(package, connection) == CORRECT) {
                debu("ALIVE_INFO: checked good\n", debug, 8);
                if (package.type == ALIVE_ACK) {
                    alive = 0;
                    debu("ALIVE_INFO: recieved alive ack, state is now ALIVE\n",
                        debug, 8);
                    connection.state = ALIVE;
                } else if (package.type == ALIVE_REJ) {
                    debu("ALIVE_INFO: recived alive rej, will proceed to shutdown\n", debug, 1);
                    boolean = 1; /* Should make it better */
                }
            } else {
                debu("ALIVE_INFO: data wasn't correctly send\n", debug, 8);
            }
        }
        udp_send(connection, ALIVE_INF, data, debug);
        alive++;
    }
    debu("Packages alive lost, state pass to disconnected\n", debug, 1);
    connection.state = DISCONNECTED;
    debu("PIPE_PRNT: send msg to child to disconnect\n", debug, 3);
    child = 'a';
    if(write(pipes[1][1], (void *) &child, sizeof(char)) == -1) {
        perror("PRNT: Error writting");
        exit(-2);
    }
    close(pipes[0][0]);
    close(pipes[1][1]);
    if (wait(NULL) == -1 ) {
        perror("Wait failed");
        exit(-1);
    }
    debu("PIPE_PRNT: child disconnected\n", debug, 3);
}


/* TODO: correct pipe and > msg (fill buffer) */
void cli(connection connection, Arg arg, int pipes[2][2]) {
    fd_set rfds;
    char parent;
    FD_ZERO(&rfds);
    FD_SET(pipes[1][0], &rfds);
    FD_SET(STDIN_FILENO, &rfds);
    printf("> ");
    fflush(stdout);
    select(pipes[1][0] + 1, &rfds
            , NULL, NULL, NULL);
    while(!FD_ISSET(pipes[1][0], &rfds)) {
        printf("\n"); 
        switch(getcommand()) {
            case SEND:
                debu("Send protocol initialitzated\n", arg.debug, 1);
                send_prot(connection, arg);
                debu("Send protocol finished\n", arg.debug, 1);
                break;
            case RECV:
                debu("GET protocol initialitzated\n", arg.debug, 1);
                get_prot(connection, arg);
                debu("GET protocol finished\n", arg.debug, 1);
                break;
            case QUIT:
                debu("Quit protocol\n", arg.debug, 1);
                debu("PIPE_CHLD: send msg to parent to disconnect\n", arg.debug, 3);
                parent = 'a';
                if(write(pipes[0][1], (void *) &parent, sizeof(char)) == -1) {
                    perror("CHLD: Error writting");
                    exit(-2);
                }
                close(pipes[0][1]);
                close(pipes[1][0]);
                debu("CHLD: Disconnected\n", arg.debug, 3);
                exit(0);
                break; /* Unreachable line, as quit exits*/
            default:
                printf("Not a client build function. "
                     "Built-in commands are: send-conf," 
                     "get-conf and quit\n");
                break;
        }
        FD_ZERO(&rfds);
        FD_SET(pipes[1][0], &rfds);
        FD_SET(STDIN_FILENO, &rfds);
        printf("> ");
        fflush(stdout);
        select(pipes[1][0] + 1, &rfds
                , NULL, NULL, NULL);
    }
    debu("\nPIPE_CHLD: CLI is shutting down\n", arg.debug, 1);
    close(pipes[0][1]);
    close(pipes[1][0]);
    exit(0);
}

/* TODO: I don't like it, redo*/
int getcommand() {
    char buffer[256];
    char *word;
    scanf("%s", buffer);
    word = strtok(buffer, " ");
    if(strcmp(word, "send-conf") == 0) {
        return SEND;
    } else if(strcmp(word, "get-conf") == 0) {
        return RECV;
    } else if(strcmp(word, "quit") == 0) {
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
             "You must restart manually\n", debug, 1);
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
    return send(sock, &package, sizeof(package), 0);
}

int tcp_package_checker(tcp_package package, connection connection) {
    if(strcmp(package.name, connection.server.nom) != 0){
        return NAME;
    } else if(strcmp(package.mac, connection.server.mac) != 0) {
        return MAC;
    } else if(strcmp(package.random, connection.random) != 0) {
        return RANDOM;
    } else {
        return CORRECT;
    }
}

int tcp_recv(int socket, tcp_package *package) {
    return recv(socket, (void *) package, sizeof(*package), 0);
}

void send_prot(connection connection, Arg arg) {
    int socket;
    tcp_package package;
    struct timeval tv;
    fd_set rfds;
    char namecfg[7+4], data[150];
    char *size;

    strcpy(namecfg, connection.nom);
    strcpy(data, arg.file);
    strcat(data, ",");
    size = getsize(arg.file);
    strcat(data, size);
    free(size);
    socket = tcp_connection(connection, arg.debug);
    debu("SEND_PROT: Send SEND_FILE package\n", arg.debug, 4);
    tcp_send(connection, socket, SEND_FILE, data);
    tv.tv_sec = W;
    FD_ZERO(&rfds);
    FD_SET(socket, &rfds);
    select(socket + 1, &rfds
            , NULL, NULL, &tv);
    if (FD_ISSET(socket, &rfds)) {
        tcp_recv(socket, &package);
        debu_tcp_package(package, arg.debug, 5);
        if(tcp_package_checker(package, connection) == CORRECT) {
            if(package.type == SEND_ACK) {
                if(strcmp(package.data, strcat(namecfg, ".cfg")) == 0) {
                    send_file(connection, socket, arg);
                } else {
                    printf("ERROR_SEND: DATA Name was not according to this node\n");
                }
            } else {
                printf("ERROR_SEND: Package type was not SEND_ACK: %x", package.type);
            }
        } else {
            printf("ERROR_SEND: Camps were not send accordingly\n");
        }

    } else {
        printf("ERROR_SEND: A trouble has occured during connexion with server."
               "to try another time, please put the command\n");
    }
    debu("SEND_PROT: closing tcp connection\n", arg.debug, 3);
    close(socket);
}


void send_file(connection connection, int socket, Arg arg) {
    FILE *file;
    char *line;
    file = fopen(arg.file, "r");
    line = (char *) malloc(150*sizeof(char));
    while(fgets(line, 150, file) != NULL) {
        tcp_send(connection, socket, SEND_DATA, line);
        debu("SEND_INFO: sending data to server...\n", arg.debug, 5);
    }
    debu("SEND_INFO: sending end to server\n", arg.debug, 3);
    line[0]='\0'; /* so it sends a void string*/
    fclose(file);
    tcp_send(connection, socket, SEND_END, line);   
}


void get_prot(connection connection, Arg arg) {
    int socket;
    tcp_package package;
    struct timeval tv;
    fd_set rfds;
    char namecfg[7+4], data[150];
    char *size;
    strcpy(namecfg, connection.nom);
    strcpy(data, arg.file);
    strcat(data, ",");
    size = getsize(arg.file);
    strcat(data, size);
    free(size);
    socket = tcp_connection(connection, arg.debug);
    debu("GET_PROT: send GET_FILE package\n", arg.debug, 3);
    tcp_send(connection, socket, GET_FILE, data);
    
    tv.tv_sec = W;
    tv.tv_usec = 0;
    FD_ZERO(&rfds);
    FD_SET(socket, &rfds);
    select(socket + 1, &rfds
            , NULL, NULL, &tv);
    debu("GET_PROT: recived package or timedout\n", arg.debug, 3); /*Unreachable line */
    if (FD_ISSET(socket, &rfds)) {
        tcp_recv(socket, &package);
        debu_tcp_package(package, arg.debug, 5);
        if(tcp_package_checker(package, connection) == CORRECT) {
            if(package.type == GET_ACK) {
                if(strcmp(package.data, strcat(namecfg, ".cfg")) == 0) {
                    get_file(connection, socket, arg);
                } else {
                    printf("ERROR_GET: Name camp was not according to this node\n");
                }
            } else {
                printf("ERROR_GET: Package type was not GET_ACK: %x\n",
                         package.type);
            }
        } else {
            printf("ERROR_GET: Camps were not send accordingly\n");
        }
    } else {
        printf("ERROR_GET: A trouble has occured during connexion with server."
               "to try another time, please put the command\n");
    }
    debu("GET_PROT: closing tcp connection\n", arg.debug, 3);
    close(socket);
}


void get_file(connection connection, int socket, Arg arg) {
    FILE *file;
    tcp_package package;
    struct timeval tv;
    fd_set rfds;
    short boolean = 0;
    tv.tv_sec = W;
    tv.tv_usec = 0;
    FD_ZERO(&rfds);
    FD_SET(socket, &rfds);
    select(socket + 1, &rfds
            , NULL, NULL, &tv);
    file = fopen(arg.file,  "w");
    tcp_recv(socket, &package); 
    while(package.type != GET_END && boolean == 0) {
        /* I don't know if the server sends or not sends \n*/
        fprintf(file, "%s", package.data); 
        debu("GET_INFO: getting data from server...\n", arg.debug, 5);
            tv.tv_sec = W;
        tv.tv_usec = 0;
        FD_ZERO(&rfds);
        FD_SET(socket, &rfds);
        select(socket + 1, &rfds
                , NULL, NULL, &tv);
        if(FD_ISSET(socket, &rfds)) {
            tcp_recv(socket, &package);
        } else {
            boolean = 1;
            debu("GET_INFO: timeout\n", arg.debug, 5);
        }
    }
    debu("GET_INFO: ended got protocol\n", arg.debug, 5);
    fclose(file);
}

char* getsize(char *path) {
    struct stat statbuf;
    char *string;
    string = malloc(sizeof(char)*7);
    if(stat(path, &statbuf) != 0) {
        perror("Error in stat file");
        /* exit(-3) * I don't know if i shouldn't comment this */
    }
    sprintf(string, "%li", statbuf.st_size);
    return string;
}

