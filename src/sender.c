#include "sender.h"
#include <poll.h>
#include "packet_interface.h"
#include "real_address.h"
#include "create_socket.h"
#include "queue_sender.h"
#include <time.h>

static int size_buffer = 1;
static int timer = 3; //define a random value for the first time, to be sure receive the first packet
static uint8_t seqNum = 0;



/**
 * Create a new structure pkt.
 *
 * @param f is a payload to be added to the structure pkt.
 *
 * @return If the payload is succesfully added onto the pkt, returns pkt. If not, the returned value is NULL.
 */
pkt_t *create_packet(char *payload, pkt_t *pkt, int len){

    pkt_set_type(pkt, PTYPE_DATA);
    pkt_set_tr(pkt, 0);
    pkt_set_timestamp(pkt, 0);
    if(pkt_set_window(pkt, 1)!=PKT_OK){
        fprintf(stderr, "sender : create_packet : error with window\n");
        return NULL;
    }
    fprintf(stderr, "create_packet : seqNum : %u\n", seqNum);
    pkt_set_seqnum(pkt, seqNum++); // does %(2^8) since type(seqNum) : uint8_t

    if(pkt_set_length(pkt, len)!=PKT_OK){
        fprintf(stderr, "sender : create_packet : error with length\n");
        return NULL;
    }
    if(pkt_set_payload(pkt, payload, pkt->length)!=PKT_OK){
        fprintf(stderr, "sender : create_packet : error with payload\n");
        return NULL;
    }
    return pkt;
}


/**
 *Loop reading a socket and printing to stdout,
 *while reading stdin and writing to the socket
 *
 * @param The socket file descriptor. It is both bound and connected.
 *        And a file descriptor with the information to send
 *
 */
void read_write_loop(const int sfd, int fd){
    uint8_t seqnum_delete; // sequence number of the payload to delete
    uint8_t seqnum_nack; // sequence number of the payload to resend
    int firstAck = 1; // indicates if first ack so that we can compute the retransmission timer
    int isLastAckNum = 0;

    char buf_ack[12];//buffer for (n)ack
    queue_t *buf_structure = queue_init();//stock all structures to send
    if(buf_structure == NULL){
        fprintf(stderr,"sender : read_write_loop : buf_structure malloc error \n");
        return;
    }


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
            fprintf(stderr, "sender : read_while_loop : error with poll : %s\n", strerror(errno));
            return;
        }

        //try to write to the socket
        if(pfds[1].revents & POLLOUT && isLastAckNum == 0){

            if(buf_structure->size < size_buffer){
                size_t len = 528;
                char *buf = (char*)malloc(528);
                char *new_payload=(char *)malloc(MAX_PAYLOAD_SIZE);
                struct timespec *tp = malloc(sizeof(struct timespec));
                if(tp == NULL){
                    fprintf(stderr, "sender : read_while_loop : error with malloc!\n");
                }

                //try to have a new payload
                fprintf(stderr,"before read\n");
                int rd = read(fd, (void *)new_payload, MAX_PAYLOAD_SIZE);
                fprintf(stderr,"after read\n");
                if(rd == -1){
                    fprintf(stderr, "sender : read_while_loop : error with read!\n");
                }
                else if(rd == 0){// && queue_isempty(buf_structure)){
                    fprintf(stderr, "There is no more payloads to read!\n"); // and buffer is empty!\n");
                    pkt_t* pkt = pkt_new();
                    if(pkt == NULL){
                        fprintf(stderr, "sender : read_while_loop : error with create_packet! \n");
                    }
                    pkt_set_type(pkt, PTYPE_DATA);
                    //TODO : if doesn't go well ? should never happen
                    //TODO : pkt_set_window ? and timestamp ?
                    pkt_set_seqnum(pkt, seqNum++); // does %(2^8) since type(seqNum) : uint8_t
                    //seqNum ++ because last ack's number will be seqNum+1
                    isLastAckNum = 1;
                    if(pkt_encode(pkt, buf, &len)!=PKT_OK){
                        fprintf(stderr, "sender : read_while_loop : error with encode\n");
                    }
                    clock_gettime(CLOCK_REALTIME, tp);
                    if(queue_push(buf_structure, pkt, tp)==-1){
                        fprintf(stderr, "sender : read_while_loop : error with push\n");
                    }

                    int wr = write(sfd, (void*)buf, len);
                    if(wr == -1){
                        fprintf(stderr, "sender : read_while_loop : error with write : %s\n", strerror(errno));
                    }

                    free(buf);
                    free(new_payload);

                    fprintf(stderr, "Wrote the stop pkt\n");
                    continue;
                }

                //encode a new structure
                pkt_t* pkt = pkt_new();
                if(pkt == NULL){
                    fprintf(stderr, "sender : read_while_loop : error with create_packet! \n");
                }
                pkt = create_packet(new_payload, pkt, rd);
                if(pkt_encode(pkt, buf, &len)!=PKT_OK){
                    fprintf(stderr, "sender : read_while_loop : error with encode\n");
                }
                clock_gettime(CLOCK_REALTIME, tp);
                if(queue_push(buf_structure, pkt, tp)==-1){
                    fprintf(stderr, "sender : read_while_loop : error with push\n");
                }


                int wr = write(sfd, (void*)buf, len);
                if(wr == -1){
                    fprintf(stderr, "sender : read_while_loop : error with write : %s\n", strerror(errno));
                }

                free(buf);
                free(new_payload);
            } // end if(buf_structure->size < size_buffer)

            //check if there is still element that wasn't resent or their timer is out
            else{
                node_t *run = buf_structure->head;
                while(run!=NULL){

                    struct timespec *tp = malloc(sizeof(struct timespec));
                    clock_gettime(CLOCK_REALTIME, tp);
                    int time_now = tp->tv_sec + (tp->tv_nsec)/1000000000;
                    if((time_now - (run->tp->tv_sec + (run->tp->tv_nsec)/1000000000)  > timer) || (run->tp->tv_sec == 0)){
                        // condition run->tp->tv_sec == 0 will happen when we receive a NACK
                        // because we set tv_sec to 0 when we receive a NACK.
                        // it won't be true otherwise because 0 sec is on the 1st of January 1970

                        fprintf(stderr, "\n\n\nI'm in the if run->tp...\n\n\n");
                        size_t len = 528;
                        char *buf = (char*)malloc(528);
                        if(buf==NULL){
                            fprintf(stderr, "sender : read_while_loop : error with malloc\n");
                        }

                        clock_gettime(CLOCK_REALTIME, tp);
                        run->tp = tp;

                        if(pkt_encode(run->pkt, buf, &len)!=PKT_OK){
                            fprintf(stderr, "sender : read_while_loop : error with encode\n");
                        }
                        int wr = write(sfd, buf, len);
                        if(wr == -1){
                            fprintf(stderr, "Stdin->socket : Write error : %s\n", strerror(errno));
                        }
                        free(buf);
                        break;
                    }
                    run = run->next;
                }
            }
        }


        //try to read the socket
        if(pfds[1].revents & POLLIN){
            fprintf(stderr, "I'm in\n");
            //analyse the (n)ack
            pkt_t* pkt_ack = pkt_new();
            int rd = read(sfd, (void*)buf_ack, 12);
            if(rd == -1){
                fprintf(stderr, "Socket->Stdout : Read error : %s\n", strerror(errno));
            }
            pkt_status_code code = pkt_decode(buf_ack, 12, pkt_ack);
            if(code == PKT_OK){
                if(isLastAckNum == 1 && pkt_ack->seqNum == seqNum){
                    fprintf(stderr, "pkt_ack seqNum : %u, isLastAckNum : %d, lastAckNum : %u\n", pkt_ack->seqNum, isLastAckNum, seqNum);
                    return;
                }
                fprintf(stderr, "pkt_ack->seqNum : %u, isLastAckNum : %d, lastAckNum : %u\n",pkt_ack->seqNum, isLastAckNum, seqNum);
                fprintf(stderr, "Decoded pkt, OK\n");
                //we have an ack
                if(pkt_ack->type == 2){
                    fprintf(stderr, "pkt_ack : PTYPE_ACK\n");
                    seqnum_delete = pkt_ack->seqNum-1;
                    //setting the max buffer size
                    size_buffer = pkt_get_window(pkt_ack);
                    //if there is a packet with 0 seqnum, reset the timer
                    queue_print_first_last_seqNum(buf_structure);
                    fprintf(stderr, "ack seqnum : %u, get_seqnum : %u\n", pkt_ack->seqNum, pkt_get_seqnum(pkt_ack));

                    if(firstAck){ //TODO : que pour le premier

                        struct timespec *tp = malloc(sizeof(struct timespec));

                        fprintf(stderr, "before queue_find_nack_structure\n");
                        struct node *time_node = queue_find_nack_structure(buf_structure, pkt_ack->seqNum-1);
                        if(time_node==NULL){
                            fprintf(stderr, "there is no structure in buffer with seqnum %u\n", pkt_ack->seqNum-1);
                            continue; // TODO : verify
                        }
                        fprintf(stderr, "after queue_find_nack_structure\n");

                        clock_gettime(CLOCK_REALTIME, tp);
                        fprintf(stderr, "after clock gettime\n");
                        int time_now = tp->tv_sec + (tp->tv_nsec)/1000000000;

                        fprintf(stderr, "before accessing time_node->tp\n");
                        timer = 2+(time_now - time_node->tp->tv_sec + time_node->tp->tv_nsec);
                        fprintf(stderr, "after accessing time_node->tp\n");
                        firstAck = 0;
                    }
                    //delete the packet
                    fprintf(stderr, "before delete\n");

                    // Useful variables to know if the pkt's seqNum lies in the window or not
                    // uint8_t so that 1-2 = 255 for example.
                    uint8_t distInf = seqnum_delete - buf_structure->head->pkt->seqNum; // distance between the beginning of the window and the seqNum
                    uint8_t distSup = (buf_structure->head->pkt->seqNum+size_buffer-1) - seqnum_delete; // distance between the end of the window and the seqNum

                    if(buf_structure->size != 0 && distInf < size_buffer && distSup < size_buffer){
                        fprintf(stderr, "Before queue_delete with seqnum_delete = %u\nLet's print the queue before !", seqnum_delete);
                        queue_print_seqNum(buf_structure);
                        if(queue_delete(buf_structure, seqnum_delete)==0){
                            fprintf(stderr, "there is no payload in buffer with seqnum %u\n", seqnum_delete);
                        }
                    }
                    else{
                        fprintf(stderr, "empty buffer or pkt not in window\n");
                    }
                }
                fprintf(stderr, "after type == 2\n");
                //we have a nack
                if(pkt_ack->type == 3){
                    fprintf(stderr, "pkt_ack : PTYPE_NACK\n");
                    seqnum_nack = pkt_ack->seqNum;
                    if(queue_find_nack_structure(buf_structure, seqnum_nack)==NULL){
                        fprintf(stderr, "there is no structure in buffer with seqnum %u\n", seqnum_nack);
                    }
                }
            }
            else{
                fprintf(stderr, "Decode pkt not ok, code : %d\n", code);
            }
        }
    }
    free(buf_structure);
    return;
}


int main(int argc, char *argv[]){

    int fd = STDIN_FILENO;//file from command line
    char *res_hostname;//hostname from command line
    char *ser_hostname = "::1";
    int dst_port;//port from command line
    int socket_fd;//socket file descriptor
    int f_option = 0;//if there is (not) f_option

    //check if there is enough arguments to continue
    if (argc<2){
        fprintf(stderr, "sender : main : error with arguments : not enough arguments!\n");
        return EXIT_FAILURE;
    }

    //register file if there are an -f option
    int opt;
    while ((opt = getopt(argc, argv, "f:")) != -1){
        switch(opt){
            case 'f':
                f_option = 1;
                fd = open(optarg, O_RDONLY);
                if(fd == -1){
                    fprintf(stderr, "sender : main : error in open \n");
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
    }


    //Resolve the resource name to an usable IPv6 address for receiver
    struct sockaddr_in6 dest_addr;
    const char *check_message = real_address(res_hostname, &dest_addr);
    if(check_message){
        fprintf(stderr, "sender : main : error in real_address : %s!\n",check_message);
    }
    //Resolve the resource name to an usable IPv6 address for sender
    struct sockaddr_in6 source_addr;
    check_message = real_address(ser_hostname, &source_addr);
    if(check_message){
        fprintf(stderr, "sender : main : error in real_address : %s!\n",check_message);
    }

    //Creates a socket and initializes it

    socket_fd = create_socket(NULL, -1, &dest_addr, dst_port);
    if(socket_fd == -1){
        fprintf(stderr, "sender : main : error in create_socket\n");
        exit(EXIT_FAILURE);
    }

    read_write_loop(socket_fd, fd);

    //TODO : Envoyer la déconnection

    close(socket_fd);
    close(fd);
    return 0;
}
