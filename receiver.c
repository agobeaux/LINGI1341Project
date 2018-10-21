#include "sender.h"
#include <poll.h>
#include "Q4 INGInious/packet_implem.c"
#include "Q3 INGInious/real_address.c"
#include "Q3 INGInious/create_socket.c"
#include "Q3 INGInious/wait_for_client.c"
#include "queue_receiver.c"

#define ser_PORT 12345 // to change. Should be an argument

//TODO : numseq à garder en mémoire, window : nombre élts restants, queue + vérification écriture
//écrire sur le fichier (stdout etc)
//demande de déconnexion à faire
// timestamp ? après 2000ms, assuré que le packet disparaît ou il faut le discard ?

//TODO : déconnexion
void read_write_loop(const int sfd, const int fd){
    struct pollfd pfds[1];
    pfds[0].fd = sfd;
    pfds[0].events = POLLIN|POLLOUT;
    pfds[0].revents = 0;

    int finishedTransfer = 0; // to know when to exit
    uint8_t waitedSeqNum = 0; // first packet has seqNum 0. uint8_t so that we won't need %(2^8)
    queue_t *pktQueue = queue_init();
    if(pktQueue == NULL){
        fprintf(stderr, "receiver : read_write_loop : pktQueue : queue_init() error\n");
        return;
    }
    queue_t *ackQueue = queue_init();
    if(pktQueue == NULL){
        fprintf(stderr, "receiver : read_write_loop : ackQueue : queue_init() error\n");
        free(pktQueue);
        return;
    }
    while(1){
        if(poll(pfds, 1, 5) == -1){
            fprintf(stderr, "Error poll call : %s\n", strerror(errno));
        }
        //fprintf(stderr, "pfds[0].revents&POLLOUT : %d, &POLLIN : %d\n", pfds[0].revents&POLLOUT, pfds[0].revents&POLLIN);
        while(pfds[0].revents&POLLOUT && ackQueue->size > 0){ // write on socket
            fprintf(stderr, "Writing on the socket\n");
            pkt_t *ack = queue_pop(ackQueue);
            if(ack == NULL){
                fprintf(stderr, "receiver : read_write_loop, ack == NULL when queue_pop(ackQueue)\n");
                continue; //TODO : continue or break ? ack shouldn't be null if queue isn't empty
            }
            char ackBuf[12];
            size_t ackLen = 12;
            if(pkt_encode(ack, ackBuf, &ackLen) != PKT_OK){
                free(ack);
                fprintf(stderr, "encode error in receiver, read_write_loop, encode != PKT_OK\n");
            }
            else if(ackLen != 12){
                fprintf(stderr, "receiver : read_write_loop, ackLen != 12. ack not send.\n");
            }
            else{
                fprintf(stderr, "I'm really writing\n");
                int wr = write(sfd, ackBuf, ackLen);
                fprintf(stderr, "Wrote %d bytes\n", wr);
                if(wr == -1){
                    fprintf(stderr, "code : %d, %s\n", errno, gai_strerror(errno));
                }
            }
        }
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

        if(pfds[0].revents&POLLIN){
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
                pkt_status_code code = pkt_decode(buf, rd, pkt);
                if(code != PKT_OK){
                    fprintf(stderr, "Receiver : read_write_loop : pkt_decode error : code : %d\n",code);

                    pkt_t *ack = pkt_new();
                    if(!ack){
                        fprintf(stderr, "Receiver : read_write_loop : creating ack, malloc error\n");
                        pkt_del(pkt);
                        break;
                    }
                    ack->type = PTYPE_ACK;
                    ack->window = 31 - pktQueue->size; // TODO : test
                    ack->seqNum = waitedSeqNum;
                    ack->timestamp = pkt->timestamp;
                    pkt_del(pkt);
                    queue_push(ackQueue, ack);
                }
                else if(pkt->trFlag == 1){
                    // truncated packet, discard it and send ack
                    pkt_t *nack = pkt_new();
                    if(!nack){
                        fprintf(stderr, "Malloc error in receiver, read_write_loop, creating nack\n");
                        pkt_del(pkt);
                        break;
                    }
                    nack->type = PTYPE_NACK;
                    nack->window = 31 - pktQueue->size; // TODO : test
                    nack->seqNum = pkt->seqNum;
                    nack->timestamp = pkt->timestamp;
                    pkt_del(pkt);
                }
                else{
                    // packet received, add it to queue
                    if(queue_ordered_push(pktQueue, pkt) == 0){
                        // pkt successfully added on the queue
                        waitedSeqNum += queue_payload_write(pktQueue, fd, waitedSeqNum);
                    }
                    else{
                        fprintf(stderr, "receiver : read_write_loop : couldn't push pkt on pktQueue\n");
                        //TODO : WARNING : it sends the ack with timestamp but the packet isn't stocked in this else...
                    }
                    pkt_t *ack = pkt_new();
                    if(!ack){
                        fprintf(stderr, "Malloc error in receiver, read_write_loop, creating ack\n");
                        break;
                    }
                    ack->type = PTYPE_ACK;
                    ack->window = 31 - pktQueue->size; // TODO : test
                    ack->seqNum = waitedSeqNum;
                    ack->timestamp = pkt->timestamp;
                    queue_push(ackQueue, ack);
                }
            }
        }
    }
}

//TODO : ne pas oublier : taille de fenêtre 2^(n-1) au max.
// comment la taille de fenêtre varie ? Dépend de nous ? sert à quoi ?
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
        res_hostname = argv[3];
        src_port = atoi(argv[4]);
        fprintf(stderr, "I'm leaving f option!\n");
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

    if (socket_fd > 0 && wait_for_client(socket_fd) < 0) { /* Connected */
        fprintf(stderr,
                "Could not connect the socket after the first message.\n");
        close(socket_fd);
        return EXIT_FAILURE;
    }
    
    fprintf(stderr, "Before read_write_loop\n");
    read_write_loop(socket_fd, fd);



    close(socket_fd);
    close(fd);
    return EXIT_SUCCESS;
}
