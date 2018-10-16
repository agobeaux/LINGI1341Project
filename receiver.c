#include "receiver.h"
#include "Q3 INGInious/real_address.c" // temporary solution
#include "Q3 INGInious/create_socket.c"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

#define ser_PORT 12345 // to change. Should be an argument

void read_write_loop(const int sfd, const int fd){
    struct pollfd pfds[2];
    pfds[0].fd = sfd;
    pfds[0].event = POLLIN|POLLOUT;
    pfds[0].revents = 0;
    pfds[1].fd = fd;
    pfds[1].event = POLLOUT;
    pfds[1].revents = 0;

    int finishedTransfer = 0; // to know when to exit
    while(1){
        if(poll(pfds, 3, 5) == -1){
            fprintf(stderr, "Error poll call : %s\n", strerror(errno));
        }

        if(pfds[0].revents&POLLOUT){ // write on socket
            uint16_t typeTrWinLength;
            // Read can block -> deadlock, right ?
            int rd = read(sfd, &typeTrWinLength, sizeof(uint16_t));
            if(rd == -1){
                fprintf(stderr, "Error while reading\n");
            }
            else if(rd == 0){
                //what do we do ? EOF ?
            }
            else{
                // TODO on process tous les ACK et NACK de la queue ! on envoie tous les acks ou seulement le dernier ?
                // si on process tous les ACK et NACK, il faut vérifier qu'on a assez de place sinon on fait un appel bloquant...
                // donc il faut refaire un poll à chaque fois ??? Et si on décide de refaire
                // 1 ack puis 1 dl de payload à chaque itération, le dl prend + de temps donc
                // potentiellement un prolème car envois d'acks trop lents

                // que se passe-t-il si on veut écrire mais que tout ne s'écrit pas sur le socket ?
                // (manque de place) on nous renvoie le nbre de bytes écrits mais
                // ils sont déjà écrits dans le socket, il faudrait les supprimer..
                // ou alors trouver un poll qui prévient s'il y a assez de places...

                struct_pkt pkt* = pkt_new();
                if(!pkt){
                    fprintf(stderr, "Error malloc in read_write_loop, pkt_new\n");
                }
                // adapter pkt_decode pour qu'il calcule lui-même la longueur ? comme ça pas d'argument len ?
                //pkt_status_code pkt_decode(packagedata, NOTlen, pkt);
                // have to create a new node and put it in the sorted queue

            }

        }

        if(pfds[0].revents&POLLIN && pfds[1].revents&POLLOUT){
            // read on socket + write on fd
            // read sizeof uint16 and then know how long the packet is ?
            // use general decode
            // and if the length is wrong ?... How can we know how long the packet is...
        }
    }
}

//TODO : ne pas oublier : taille de fenêtre 2^(n-1) au max.
// comment la taille de fenêtre varie ? Dépend de nous ? sert à quoi ?
// utiliser une sorted fifo pour avoir les paquets qu'on veut print dans l'ordre
int main(int argc, char *argv[]){

    int fd; //file from command line
    char *res_hostname; //hostname from command line
    //char *ser_hostname = "::1"; //useless ?
    int src_port; //port from command line
    int socket_fd; //socket file descriptor



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
        src_port = atoi(argv[2]);
        fd = STDIN_FILENO;
    }
    else{
        fprintf(stderr, "I'm in f option!\n");
        res_hostname = argv[4];
        src_port = atoi(argv[5]);
    }


    //Resolve the resource name to an usable IPv6 address
    struct sockaddr_in6 addr;
    const char *check_message = real_address(res_hostname, &addr);
    if(check_message){
        fprintf(stderr, "Problem in getaddrinfo %s!\n",check_message);
    }

    //Creates a socket and initializes it
    socket_fd = create_socket(&addr, src_port, NULL, -1); //réecrire
    if(socket_fd == -1){
        fprintf(stderr, "Failed to create the socket!\n");
        exit(EXIT_FAILURE);
    }


    /* DO THINGS */


    close(socket_fd);
    close(fd);
    return EXIT_SUCCESS;
}
