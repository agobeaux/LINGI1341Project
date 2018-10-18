#include "receiver.h"
#include "Q3 INGInious/real_address.c" // temporary solution
#include "Q3 INGInious/create_socket.c"
#include "Q4 INGInious/packet_implem.c"
#include <poll.h>
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
    pfds[0].events = POLLIN|POLLOUT;
    pfds[0].revents = 0;
    pfds[1].fd = fd;
    pfds[1].events = POLLOUT;
    pfds[1].revents = 0;

    int finishedTransfer = 0; // to know when to exit
    while(1){
        fprintf(stderr, "In while\n");
        if(poll(pfds, 3, 5) == -1){
            fprintf(stderr, "Error poll call : %s\n", strerror(errno));
        }
        fprintf(stderr, "sfd : pollout : %d, sfd : pollin : %d, fd : pollout : %d\n",pfds[0].revents&POLLOUT,pfds[0].revents&POLLIN,pfds[1].revents&POLLOUT);
        if(pfds[0].revents&POLLOUT){ // write on socket
            fprintf(stderr, "I'm in here\n");
            // TODO on process tous les ACK et NACK de la queue ! on envoie tous les acks ou seulement le dernier ?
            // si on process tous les ACK et NACK, il faut vérifier qu'on a assez de place sinon on fait un appel bloquant...
            // donc il faut refaire un poll à chaque fois ??? Et si on décide de refaire
            // 1 ack puis 1 dl de payload à chaque itération, le dl prend + de temps donc
            // potentiellement un prolème car envois d'acks trop lents

            //QUESTIONS : 1 : poll fonctionne même s'il n'y a qu'1 byte de libre pour écrire
            // => problème, si on n'écrit pas tout, il va y avoir un problème quand l'autre va essayer de lire
            // le paquet... il faudrait pouvoir enlever ce qui a été écrit, comment ? ...
            // 2 : si on a plusieurs ACK à envoyer, on envoie normalement ou alors on fait la synthèse
            // des acks pour envoyer seulement le dernier (+ les nacks)
            // 3 : si on reçoit un paquet avec TR = 1 mais length != 0, et CRC1 est bon (alors que le paquet a été modifié)
            // si ça tombe c'est un paquet tronqué ou alors un paquet avec payload donc on devrait discard tt le socket ?
            // car pas sûr de la place sur le payload... pareil quand le paquet a un mauvais header...
            // à chaque fois il faut discard tous les paquets du socket du coup ? ...

            // REPONSES : à nous de gérer la window. -> d'abord taille fixe, ou même en argument du programme
            //            gérer le changement de taille par après
            //            poll assure qu'on peut écrire ou lire un paquet UDP entier car protocole UDP -> noworries pour écrire
            //            plusieurs acks : notre choix : plus facile de juste renvoyer d'abord mais on peut sélectionner le meilleur
            //            si on a plusieurs ack à envoyer (si 3 acks, on peut en envoyer un seul ou alors en envoyer 3 fois le meilleur)
            //            paquet tronqué ? erreur etc ? pas grave car on lit paquet par paquet grâce à UDP
            //            ATTENTION : READ on met la longueur max d'un paquet.



            // que se passe-t-il si on veut écrire mais que tout ne s'écrit pas sur le socket ?
            // (manque de place) on nous renvoie le nbre de bytes écrits mais
            // ils sont déjà écrits dans le socket, il faudrait les supprimer..
            // ou alors trouver un poll qui prévient s'il y a assez de places...

            /*
            struct_pkt pkt* = pkt_new();
            if(!pkt){
                fprintf(stderr, "Error malloc in read_write_loop, pkt_new\n");
            }
            */

            // adapter pkt_decode pour qu'il calcule lui-même la longueur ? comme ça pas d'argument len ?
            // pkt_status_code pkt_decode(packagedata, NOTlen, pkt);
            // have to create a new node and put it in the sorted queue

        }
        fprintf(stderr, "sfd : pollin : %d, fd : pollout : %d\n",pfds[0].revents&POLLIN,pfds[1].revents&POLLOUT);

        if(pfds[0].revents&POLLIN && pfds[1].revents&POLLOUT){
            fprintf(stderr,"=============== HERE ==============\n");
            // read the max size of a packet cause we'll read one packet at a time
            // we won't read a packet and then a part of another. Source : obo
            char buf[528];
            int rd = read(sfd, buf, 528);
            if(rd == -1){
                fprintf(stderr, "Error while reading\n");
            }
            else if(rd == 0){
                //what do we do ? EOF ?
                fprintf(stderr, "receiver, read_write_loop, rd == 0\n");
            }
            else{
                pkt_t *pkt = pkt_new();
                if(!pkt){
                    fprintf(stderr, "Malloc error in receiver, read_write_loop, creating pkt\n");
                    break;
                }
                int code;
                if(code = pkt_decode(buf, rd, pkt) != PKT_OK){
                    fprintf(stderr, "Decode error in receiver, read_write_loop, code : %d\n",code);
                    free(pkt);
                    break;
                }
                fprintf(stderr, "L 105\n");
                if(pkt->trFlag == 1){
                    // truncated packet, discard it and send ack
                    pkt_t *nack = pkt_new();
                    if(!nack){
                        fprintf(stderr, "Malloc error in receiver, read_write_loop, creating nack\n");
                        free(pkt);
                        break;
                    }
                    nack->type = PTYPE_NACK;
                    nack->window = 31; // TODO : test
                    nack->seqNum = pkt->seqNum;
                    //TODO : only info ?? timestamp !
                    free(pkt);
                    char nackBuf[12];
                    size_t pktLen = 12;
                    if(pkt_encode(nack, nackBuf, &pktLen) != PKT_OK){
                        free(nack);
                        fprintf(stderr, "encode error in receiver, read_write_loop, nack creation\n");
                    }
                    fprintf(stderr, "I M WRITING SOMETHING\n");
                    write(sfd, nackBuf, pktLen);
                }
                else{
                    // truncated packet, discard it and send ack
                    pkt_t *ack = pkt_new();
                    if(!ack){
                        fprintf(stderr, "Malloc error in receiver, read_write_loop, creating ack\n");
                        free(pkt);
                        break;
                    }
                    ack->type = PTYPE_ACK;
                    ack->window = 31; // TODO : test
                    ack->seqNum = pkt->seqNum+1;
                    free(pkt);
                    char ackBuf[528];
                    size_t pktLen = 12;
                    if(pkt_encode(ack, ackBuf, &pktLen) != PKT_OK){
                        free(ack);
                        fprintf(stderr, "encode error in receiver, read_write_loop, ack creation\n");
                    }
                    fprintf(stderr, "I M WRITING SOMETHING\n");
                    write(sfd, ackBuf, pktLen);
                }

            // read on socket + write on fd
            // read sizeof uint16 and then know how long the packet is ?
            // use general decode
            // and if the length is wrong ?... How can we know how long the packet is...
            }
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
        fd = STDOUT_FILENO;
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
    read_write_loop(socket_fd, fd);

    close(socket_fd);
    close(fd);
    return EXIT_SUCCESS;
}
