#include "sender.h"
#include "Q4 INGInious/packet_implem.c"
#include "Q3 INGInious/real_address.c"
#include "Q3 INGInious/create_socket.c"
#include "stack.c"


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

int main(int argc, char *argv[]){
    
    int fd;//file from command line
    char *res_hostname;//hostname from command line
    char *ser_hostname = "::1";
    int dst_port;//port from command line
    int socket_fd;//socket file descriptor
    int can_read = 1;//can continue to read from file
    int f_option = 0;//if there is (not) f_option
    char *new_payload=(char *)malloc(MAX_PAYLOAD_SIZE);
    int err;
    int window_size=5;//for this moment window is fixe and equal to 5
    size_t len = 528;
    char *buf = (char*)malloc(528);
    
    
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
        //int read = getline(&info, &length, stdin);
        //if(read==-1){
           // fprintf(stderr, "Problem in geting line stdin!\n");
        //}
        //fprintf(stderr, "Info we are getting from stdin %s!\n", info);
    }
    else{
        fprintf(stderr, "I'm in f option!\n");
        res_hostname = argv[3];
        dst_port = atoi(argv[4]);
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
    //faire buffer pour plusieurs payload
    //window taille fixe
    //retransmission timer
    //implementation of while
    //while(true){
        //lire dans le fichier
    
    
    //pour le moment traiter qu'un seul payload reçu
        if(can_read){
            //if we have a file
            if(f_option){
                err = read(fd, (void *)new_payload, MAX_PAYLOAD_SIZE);
                if(err == -1){
                    fprintf(stderr, "Failed to read a new payload!\n");
                    exit(EXIT_FAILURE);
                }
            }
            //if we must read from stdin
            else{
                err = read(0, (void *)new_payload, MAX_PAYLOAD_SIZE);
                if(err == -1){
                    fprintf(stderr, "Failed to read a new payload!\n");
                    exit(EXIT_FAILURE);
                }
            }
            //when we have a payload, create a packet
            pkt_t* pkt = pkt_new();
            pkt = create_packet(new_payload, pkt);
            pkt_encode(pkt, buf, &len);
            write(socket_fd,(void *) buf, &len + 16);
        }
    
    //}
    
    
    //TODO
    //traitement de signal, reception de ack, nack, traitement de signal
    //appel bloquant pour le sender, quand il arrete à read? avec quoi?
    
    
    
    close(socket_fd);
    close(fd);
    free(new_payload);
    free(buf);
    return 0;
}
