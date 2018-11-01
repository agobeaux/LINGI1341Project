#include "sender.h"
#include <poll.h>

#include "real_address.h"
#include "create_socket.h"
#include "queue_sender.h"
#include <time.h>

static int size_buffer = 1; //buffer's size limitation (limited by the value of window)
static int timer = 3; //the transmission timeout (3s is the default value)
static uint8_t seqNum = 0; //seqNum of the next packet to send
struct timespec *tpGlobal; //timer to disconnect if there is no more information to send


/**
 * Initialize a new packet.
 *
 * @pkt packet to initialize
 * @payload payload to add to the packet
 * @payload the payload's length
 *
 * @return : pkt if the payload is succesfully added onto the pkt, Null otherwise.
 */
pkt_t *create_packet(char *payload, pkt_t *pkt, int len){

    pkt_set_type(pkt, PTYPE_DATA);
    pkt_set_tr(pkt, 0);

    struct timespec *timePkt = malloc(sizeof(struct timespec));
    if(!timePkt){
        fprintf(stderr, "sender : create_packet : couldn't malloc tpGlobal\n");
        return NULL;
    }
    clock_gettime(CLOCK_REALTIME, timePkt);

    pkt_set_timestamp(pkt, timePkt->tv_sec);
    free(timePkt);

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
 *Loop for writing packets and reading the acks/nacks
 *
 * @sfd socket's file descriptor
 * @fd file descriptor containing the informations to send
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


    struct pollfd pfds[1];
    pfds[0].fd = sfd;
    pfds[0].events = POLLIN|POLLOUT;
    pfds[0].revents = 0;

    while(1){

		//check if there are still some exchange between sender and receiver, if it is not the case then close the socket
		struct timespec *tpNow = malloc(sizeof(struct timespec));
		clock_gettime(CLOCK_REALTIME, tpNow);
		int difftime = tpNow->tv_sec - tpGlobal->tv_sec;
		if(difftime>10){
			return;
		}

		free(tpNow);


        //creation of poll
        int pRet = poll(pfds, 1, 500);
        if(pRet == -1){
            fprintf(stderr, "sender : read_while_loop : error with poll : %s\n", strerror(errno));
            return;
        }

        //try to write to the socket
        if(pfds[0].revents & POLLOUT){
            if(buf_structure->size < size_buffer && isLastAckNum == 0){
                size_t len = 528;
                char *buf = (char*)malloc(528);
                char *new_payload=(char *)malloc(MAX_PAYLOAD_SIZE);

                //read a new payload
                fprintf(stderr,"before read\n");
                int rd = read(fd, (void *)new_payload, MAX_PAYLOAD_SIZE);
                fprintf(stderr,"after read\n");
                fprintf(stderr, "READ : %d\n",rd);
                if(rd == -1){
                    fprintf(stderr, "sender : read_while_loop : error with read!\n");
                }
                else if(rd == 0){//no more information to send

					//reset the transmission timer because there is still something to write
					clock_gettime(CLOCK_REALTIME, tpGlobal);

                    fprintf(stderr, "There is no more payloads to read!\n"); // and buffer is empty!\n");
                    pkt_t* pkt = pkt_new();
                    if(pkt == NULL){
                        fprintf(stderr, "sender : read_while_loop : error with create_packet! \n");
                        continue;
                    }
                    
                    pkt_set_type(pkt, PTYPE_DATA);
                    pkt_set_seqnum(pkt, seqNum++);

                    struct timespec *timePkt = malloc(sizeof(struct timespec));
                    if(!timePkt){
                        fprintf(stderr, "sender : create_packet : couldn't malloc tpGlobal\n");
                        return;
                    }
                    clock_gettime(CLOCK_REALTIME, timePkt);

                    pkt_set_timestamp(pkt, timePkt->tv_sec);
                    free(timePkt);

                    isLastAckNum = 1;
                    if(pkt_encode(pkt, buf, &len)!=PKT_OK){
                        fprintf(stderr, "sender : read_while_loop : error with encode\n");
                    }
                    if(queue_push(buf_structure, pkt)==-1){
						free(pkt);
                        fprintf(stderr, "sender : read_while_loop : error with push\n");
                    }
                    int wr = write(sfd, (void*)buf, len);
                    if(wr == -1){
                        fprintf(stderr, "sender : read_while_loop : error with write : %s\n", strerror(errno));
                    }

                    free(buf);
                    free(new_payload);

                    fprintf(stderr, "\n\n\n\n Just after sending the 0 payload \n\n\n\n");
                    continue;
                }

                //reset the transmission timer because there is still something to write
				clock_gettime(CLOCK_REALTIME, tpGlobal);

                //encode a new structure
                pkt_t* pkt = pkt_new();
                if(pkt == NULL){
                    fprintf(stderr, "sender : read_while_loop : error with create_packet! \n");
                }
                
                pkt = create_packet(new_payload, pkt, rd);
                if(pkt_encode(pkt, buf, &len)!=PKT_OK){
                    fprintf(stderr, "sender : read_while_loop : error with encode\n");
                }
                
                if(queue_push(buf_structure, pkt)==-1){
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
			node_t *run = buf_structure->head;
			while(run!=NULL){

				struct timespec *tp = malloc(sizeof(struct timespec));
				clock_gettime(CLOCK_REALTIME, tp);
				int time_now = tp->tv_sec;
				free(tp);
				
				if(((time_now - (int)(run->pkt->timestamp)) > timer) || (run->pkt->timestamp == 0)){

					pRet = poll(pfds, 1, 500);
					if(pRet == -1){
						fprintf(stderr, "sender : read_while_loop : error with poll : %s\n", strerror(errno));
						return;
					}

					if(pfds[0].revents & POLLOUT){

						fprintf(stderr, "\n\n\n\n\n\n j'ai trouvé l'élement avec le retransmission timeout, %d \n\n\n\n\n\n", run->pkt->seqNum);
						// condition run->pkt->timestamp_sec == 0 will happen when we receive a NACK
						// because we set tv_sec to 0 when we receive a NACK.
						// it won't be true otherwise because 0 sec is on the 1st of January 1970
						size_t len = 528;
						char *buf = (char*)malloc(528);
						if(buf == NULL){
							fprintf(stderr, "sender : read_while_loop : error with malloc\n");
							continue;
						}

						struct timespec *timePkt = malloc(sizeof(struct timespec));
						if(!timePkt){
							fprintf(stderr, "sender : create_packet : couldn't malloc tpGlobal\n");
							return;
						}
						clock_gettime(CLOCK_REALTIME, timePkt);

						pkt_set_timestamp(run->pkt, timePkt->tv_sec);
						free(timePkt);

						if(pkt_encode(run->pkt, buf, &len)!=PKT_OK){
							fprintf(stderr, "sender : read_while_loop : error with encode\n");
							free(buf);
							continue;
						}
						int wr = write(sfd, buf, len);
						if(wr == -1){
							fprintf(stderr, "Stdin->socket : Write error : %s\n", strerror(errno));
						}
						free(buf);
						break;
					}
				}
				run = run->next;
			}
        }


        //try to read the socket
        if(pfds[0].revents & POLLIN){
			//reset the transmission timer because there is still something to read
			clock_gettime(CLOCK_REALTIME, tpGlobal);


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
                    pkt_del(pkt_ack);
                    queue_free(buf_structure);
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

                        fprintf(stderr, "before queue_find_ack_structure\n");
                        pkt_t *time_pkt = queue_find_ack_structure(buf_structure, pkt_ack->seqNum-1);
                        if(time_pkt==NULL){
                            fprintf(stderr, "there is no structure in buffer with seqnum %u\n", pkt_ack->seqNum-1);
                            pkt_del(pkt_ack);
                            continue; // TODO : verify
                        }
                        fprintf(stderr, "after queue_find_ack_structure\n");

                        clock_gettime(CLOCK_REALTIME, tp);
                        fprintf(stderr, "after clock gettime\n");
                        int time_now = tp->tv_sec;

                        fprintf(stderr, "before accessing time_node->tp\n");
                        fprintf(stderr, "TIMER : %d\n",timer);
                        timer = 2*(time_now - (int)(time_pkt->timestamp));
                        fprintf(stderr, "TIMER : %d\n",timer);
                        fprintf(stderr, "after accessing time_node->tp\n");
                        free(tp);
                        firstAck = 0;
                    }
                    pkt_del(pkt_ack);
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
                 //end of if(pkt_ack->type == 2)
                // we have a nack
                else if(pkt_ack->type == 3){
                    fprintf(stderr, "pkt_ack : PTYPE_NACK\n");
                    seqnum_nack = pkt_ack->seqNum;
                    if(queue_find_nack_structure(buf_structure, seqnum_nack)==NULL){
                        fprintf(stderr, "there is no structure in buffer with seqnum %u\n", seqnum_nack);
                    }
                    pkt_del(pkt_ack);
                }
            }
            else{
                fprintf(stderr, "Decode pkt not ok, code : %d\n", code);
            }
        }

    }
    queue_free(buf_structure);
    return;
}


int main(int argc, char *argv[]){

    int fd = STDIN_FILENO;//file from command line
    char *res_hostname;//hostname from command line
    int dst_port;//port from command line
    int socket_fd;//socket file descriptor
    int f_option = 0;//if there is (not) f_option
    tpGlobal = malloc(sizeof(struct timespec));
    if(!tpGlobal){
        fprintf(stderr, "sender : main : couldn't malloc tpGlobal\n");
        return 0;
    }
    clock_gettime(CLOCK_REALTIME, tpGlobal);

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


    //Creates a socket and initializes it

    socket_fd = create_socket(NULL, -1, &dest_addr, dst_port);
    if(socket_fd == -1){
        fprintf(stderr, "sender : main : error in create_socket\n");
        exit(EXIT_FAILURE);
    }

    read_write_loop(socket_fd, fd);

	free(tpGlobal);
    close(socket_fd);
    close(fd);
    return 0;
}
