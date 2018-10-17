#include "sender.h"
#include "Q4 INGInious/packet_implem.c"
#include "Q3 INGInious/real_address.c"
#include "queue.c"

//TODO Modiefier la structure pour pouvoir faire LIFO -> on ajoute la 1er élément au tete du tableau
//on retire le plus vieux élement possible

static int size_buffer = 0;


int main(int argc, char *argv[]){

    int fd;//file from command line
    char *res_hostname;//hostname from command line
    char *ser_hostname = "::1";
    int dst_port;//port from command line
    int socket_fd;//socket file descriptor
    size_t length = sizeof(char)*40;
    char *info = (char *)malloc(length); // !!!!!!!!not sure of the value



    //check if there is enough arguments to continue
    if (argc<2){
        fprintf(stderr, "There is not enough arguments!\n");
        return EXIT_FAILURE;
    }

    //register file if there are an -f option
    int opt;
    int f_option = 0;
    while ((opt = getopt(argc, argv, "f:")) != -1){
        switch(opt){
            case 'f':
                f_option = 1;
                fd = open(optarg, O_RDONLY);
                fprintf(stderr, "Find an f option!\n");
                if(fd == -1){
                    fprintf(stderr, "Can't open the file!\n");
                    exit(EXIT_FAILURE);
                }
                break;
            default:
                fprintf(stderr, "There is an unknown option!\n");
                exit(EXIT_FAILURE);
        }
    }
    //register hostname && port
    if(f_option==0){
        fprintf(stderr, "I'm in no-f option!\n");
        res_hostname = argv[1];
        dst_port = atoi(argv[2]);
        int read = getline(&info, &length, stdin);
        if(read==-1){
            fprintf(stderr, "Problem in geting line stdin!\n");
        }
        fprintf(stderr, "Info we are getting from stdin %s!\n", info);
    }
    else{
        fprintf(stderr, "I'm in f option!\n");
        res_hostname = argv[4];
        dst_port = atoi(argv[5]);
    }


    //Resolve the resource name to an usable IPv6 address for receiver
    struct sockaddr_in6 dest_addr;
    const char *check_message = real_address(res_hostname, &dest_addr);
    if(check_message){
        fprintf(stderr, "Problem in getaddrinfo %s!\n",check_message);
    }
    //Resolve the resource name to an usable IPv6 address for sender
    struct sockaddr_in6 source_addr;
    check_message = real_address(ser_hostname, &source_addr);
    if(check_message){
        fprintf(stderr, "Problem in getaddrinfo %s!\n",check_message);
    }

    //Creates a socket and initializes it
    socket_fd = create_socket(&source_addr, -1, &dest_addr, dst_port);
    if(socket_fd == -1){
        fprintf(stderr, "Failed to create the socket!\n");
        exit(EXIT_FAILURE);
    }


    /* DO THINGS */
    //envoyer les packets
    //traiter les ack
    //traiter les nack
    //appel bloquant pour le sender, quand il arrete à read? avec quoi?
    //window taille fixe
    //retransmission timer
    //


    char *buf = "Lily";
    write(socket_fd,(void *) buf, 5);

    close(socket_fd);
    close(fd);
    return 0;
}
