#include "sender.h"
#include <poll.h>
#include "Q4 INGInious/packet_implem.c"
#include "Q3 INGInious/real_address.c"
#include "Q3 INGInious/create_socket.c"
#include "queue.c"


static int size_buffer = 0;
//add  : check if it's OK,if not -> return NULL
pkt_t *create_packet(char *payload, pkt_t *pkt){
    //set data
    pkt_set_type(pkt, PTYPE_DATA);
    pkt_set_tr(pkt, 0);
    pkt_set_timestamp(pkt, 0);
    pkt_set_window(pkt, 1);
    pkt_set_seqnum(pkt, 0);
    pkt_set_length(pkt, strlen(payload));
    pkt_set_payload(pkt, payload, pkt->length);
    return pkt;
}

/* Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */
void read_write_loop(const int sfd, int fd){
    int err;
    
    char buf_ack[13];//buffer for (n)ack
    queue_t *buf_structure;//TODO
    
    struct pollfd pfds[2];
    pfds[0].fd = STDIN_FILENO;
    pfds[0].events = POLLIN;
    pfds[0].revents = 0;
    pfds[1].fd = sfd;
    pfds[1].events = POLLIN|POLLOUT;
    pfds[1].revents = 0;
    
    while(1){
        
        //creation of poll
        int pRet = poll(pfds, 2, 500);
        if(pRet == -1){
            fprintf(stderr, "Error poll call: %s\n", strerror(errno));
        }
        
        
        //try to write to the socket
        if(pfds[1].revents & POLLOUT){
            //buf to stock the structure
            size_t len = 528;
            char *buf = (char*)malloc(528);
            char *new_payload=(char *)malloc(MAX_PAYLOAD_SIZE);
            
            //try to have a new payload
            err = read(fd, (void *)new_payload, MAX_PAYLOAD_SIZE);
            if(err == -1){
                fprintf(stderr, "Failed to read a new payload!\n");
                exit(EXIT_FAILURE);
            }
            if(err == 0){
                fprintf(stderr, "There is no more payloads to read!\n");
                break;
            }
            
            //encode a new structure
            pkt_t* pkt = pkt_new();
            pkt = create_packet(new_payload, pkt);
            pkt_encode(pkt, buf, &len);
            
            //send structure
            int wr = write(sfd, (void*)buf, len);
            if(wr == -1){
                fprintf(stderr, "Stdin->socket : Write error : %s\n", strerror(errno));
            }
        }
        
        //try to read the socket
        if(pfds[1].revents & POLLIN){
            
            //analyse the (n)ack
            pkt_t* pkt_ack = pkt_new();
            fprintf(stderr, "je vais lire!\n");
            int rd = read(sfd, (void*)buf_ack, 12);
            if(rd == -1){
                fprintf(stderr, "Socket->Stdout : Read error : %s\n", strerror(errno));
            }
            pkt_decode(buf_ack, 12, pkt_ack);
            //if(pkt_ack->type == 2){
            //supprimer le packet
            //}
            //if(pkt_ack->type == 3){
            //renvoyer packet
            //}
            break;
        }
    }
    return;
}


int main(int argc, char *argv[]){

    int fd;//file from command line
    char *res_hostname;//hostname from command line
    char *ser_hostname = "::1";
    int dst_port;//port from command line
    int socket_fd;//socket file descriptor
    int f_option = 0;//if there is (not) f_option
    int err;
    int window_size=5;//for this moment window is fixe and equal to 5
    
    //check if there is enough arguments to continue
    if (argc<2){
        fprintf(stderr, "There is not enough arguments!\n");
        return EXIT_FAILURE;
    }

    //register file if there are an -f option
    int opt;
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
    }
    else{
        fprintf(stderr, "I'm in f option!\n");
        res_hostname = argv[3];
        dst_port = atoi(argv[4]);
        fd = STDIN_FILENO;
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
    
    socket_fd = create_socket(NULL, -1, &dest_addr, dst_port);
    if(socket_fd == -1){
        fprintf(stderr, "Failed to create the socket!\n");
        exit(EXIT_FAILURE);
    }


    /* DO THINGS */
    //faire buffer pour plusieurs payload
    //window taille fixe
    //retransmission timer
    //implementation of while
    //while(true){
        //lire dans le fichier
    
    
    
    
    //when we have a payload, create a packet
    read_write_loop(socket_fd, fd);
    
    //}
    
    
    //TODO
    //traitement de signal, reception de ack, nack, traitement de signal
    //appel bloquant pour le sender, quand il arrete à read? avec quoi?
    

    close(socket_fd);
    close(fd);
    return 0;
}
