#include<getopt.h>
#include<stdio.h>

#define ARG_SIZE 20

int main(int argc, char **argv) {
    int debug, c;
    char config[ARG_SIZE], file[ARG_SIZE];
    debug = 0;
    sprintf(config, "client.cfg");
    sprintf(file, "boot.cfg");
    while((c = getopt(argc, argv, "dc:f:")) != -1) {
        switch (c)
        {
            case 'd':
                debug = 1;
                break;
            case 'c':
                sprintf(config, "%s", optarg);
                break;
            case 'f':
                sprintf(file, "%s", optarg);
            default:
                break;
        }
    }
    if (optind < argc) {
        printf("Too many arguments, they will be omitted\n");
    }
}