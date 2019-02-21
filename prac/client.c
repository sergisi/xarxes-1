/* Includes done to the project */
#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* All constants */
#define CONFIG_SIZE 20

/* All enums */

/* All structs used in the program */
typedef struct arg {
    char config[CONFIG_SIZE];
    char file[CONFIG_SIZE];
    int debug;
} Arg;


/* All functions declarated */
Arg argparser(int argc, char **argv);
void debug(char *text, int debug);
int udp_sock(char config[CONFIG_SIZE]);

int main(int argc, char **argv) {
    Arg arg;
    arg = argparser(argc, argv);
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
