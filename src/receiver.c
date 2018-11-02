#include "receiver.h"
#include <poll.h>
#include "packet_interface.h"
#include "real_address.h"
#include "create_socket.h"
#include "wait_for_client.h"
#include "queue_receiver.h"
#include <time.h>

struct timespec *tpGlobal;


/**
 * Loop for writing packets and reading the acks/nacks
 *
 * @sfd socket's file descriptor
 * @fd file descriptor containing the informations to send
 */
void read_write_loop(const int sfd, const int fd){
    struct pollfd pfds[1];
    pfds[0].fd = sfd;
    pfds[0].events = POLLIN|POLLOUT;
    pfds[0].revents = 0;

    uint8_t waitedSeqNum = 0; // first packet has seqNum 0. uint8_t so that we won't need %(2^8)
    uint8_t finalSeqNum = -1;
    uint32_t lastTimeStamp = 0;
    int isFinalSeqNum = 0;
    queue_t *pktQueue = queue_init();
    if(pktQueue == NULL){
        fprintf(stderr, "receiver : read_write_loop : pktQueue : queue_init() error\n");
        return;
    }
    queue_t *ackQueue = queue_init();
    if(ackQueue == NULL){
        fprintf(stderr, "receiver : read_write_loop : ackQueue : queue_init() error\n");
        free(pktQueue);
        return;
    }
    while(1){

		//check if there are still some exchange between sender and receiver, if it is not a case then close the socket
		struct timespec *tpNow = malloc(sizeof(struct timespec));
		clock_gettime(CLOCK_REALTIME, tpNow);
		int difftime = tpNow->tv_sec - tpGlobal->tv_sec;
		if(difftime>10){
            queue_free(pktQueue);
            queue_free(ackQueue);
            free(tpNow);
			return;
		}
		free(tpNow);

        if(poll(pfds, 1, 5) == -1){
            fprintf(stderr, "Error poll call : %s\n", strerror(errno));
        }

		//try to write to the socket
        while(pfds[0].revents&POLLOUT && ackQueue->size > 0){

			//encode and send an ack
            pkt_t *ack = queue_pop(ackQueue);
            if(ack == NULL){
                fprintf(stderr, "receiver : read_write_loop, ack == NULL when queue_pop(ackQueue)\n");
                pkt_del(ack);
                continue;
            }
            int trFlag = ack->trFlag;
            ack->trFlag = 0;
            char ackBuf[ACK_SIZE];
            size_t ackLen = ACK_SIZE;
            if(pkt_encode(ack, ackBuf, &ackLen) != PKT_OK){
                pkt_del(ack);
                fprintf(stderr, "encode error in receiver, read_write_loop, encode != PKT_OK\n");
            }
            else if(ackLen != ACK_SIZE){
				pkt_del(ack);
                fprintf(stderr, "receiver : read_write_loop, ackLen != 12. ack not send.\n");
            }
            else{
                int wr = write(sfd, ackBuf, ackLen);
                pkt_del(ack);
                if(wr == -1){
                    fprintf(stderr, "code : %d, %s\n", errno, gai_strerror(errno));
                }
                if(trFlag == 1){
                    if(pktQueue->size != 0 || ackQueue->size != 0){
						fprintf(stderr, "Error : receiver.c, read_write_loop : gonna return and at least one of the queue is not empty !!!\n");
					}
                    queue_free(pktQueue);
                    queue_free(ackQueue);
                    return;
                }
            }
        }

		//try to read from the socket
        if(pfds[0].revents&POLLIN){

			//reset the transmission timer because there is still something to read
			clock_gettime(CLOCK_REALTIME, tpGlobal);

            // read the packet
            char buf[MAX_PACKET_SIZE];
            int rd = read(sfd, buf, MAX_PACKET_SIZE);
            if(rd == -1){
                fprintf(stderr, "Error while reading\n");
            }
            else if(rd == 0){
                //what do we do ? EOF ?
                fprintf(stderr, "receiver, read_write_loop, rd == 0\n");
            }
            else{
                uint8_t realWindowSize = MAX_BUFFER_SIZE;
                pkt_t *pkt = pkt_new();
                if(!pkt){
                    fprintf(stderr, "Malloc error in receiver, read_write_loop, creating pkt\n");
                    break;
                }
                pkt_status_code code = pkt_decode(buf, rd, pkt);

                // not a valid packet, discard it and send ack
                if(code != PKT_OK){

                    pkt_t *ack = pkt_new();
                    if(!ack){
                        fprintf(stderr, "Receiver : read_write_loop : creating ack, malloc error\n");
                        break;
                    }
                    ack->type = PTYPE_ACK;
                    ack->window = MAX_BUFFER_SIZE;
                    ack->seqNum = waitedSeqNum;
                    ack->timestamp = 0;
                    queue_push(ackQueue, ack);
                }
                // truncated packet, discard it and send ack
                else if(pkt->trFlag == 1){
                    pkt_t *nack = pkt_new();
                    if(!nack){
                        fprintf(stderr, "Malloc error in receiver, read_write_loop, creating nack\n");
                        pkt_del(pkt);
                        break;
                    }
                    nack->type = PTYPE_NACK;
                    nack->window = MAX_BUFFER_SIZE;
                    nack->seqNum = pkt->seqNum;
                    nack->timestamp = pkt->timestamp;
                    pkt_del(pkt);
                    queue_push(ackQueue, nack);
                }
                else{

                    // check if the pkt's seqNum is in the window or not
                    // uint8_t makes it easier to go from 255 to 0, for exemple : 1-2 = 255
                    uint8_t distInf = pkt->seqNum - waitedSeqNum; // distance between the beginning of the window and the seqNum
                    uint8_t distSup = (waitedSeqNum+realWindowSize-1) - pkt->seqNum; // distance between the end of the window and the seqNum

                    if(distInf < realWindowSize && distSup < realWindowSize){

						//if length == 0, end of the transmission
                        if(pkt->length == 0){

                            pkt_t *ack = pkt_new();
                            if(!ack){
                                fprintf(stderr, "Malloc error in receiver, read_write_loop, creating ack\n");
                                pkt_del(pkt);
                                break;
                            }
                            //create a new ack and push it on the queue 'ackQueue'
                            ack->type = PTYPE_ACK;
                            isFinalSeqNum = 1;
                            finalSeqNum = pkt->seqNum;
                            if(pkt->seqNum == waitedSeqNum){
                                ack->trFlag = 1; //TODO : I defined this to know which ack should be the last one
                                waitedSeqNum++;
                            }
                            ack->window = MAX_BUFFER_SIZE;
                            ack->seqNum = waitedSeqNum;
                            ack->timestamp = pkt->timestamp;
                            queue_push(ackQueue, ack);
                            pkt_del(pkt);
                            continue;
                        }

                        //push the new pkt on the queue 'pktQueue'
                        if(queue_ordered_push(pktQueue, pkt, waitedSeqNum, realWindowSize) == 0){
                            queue_print_seqNum(pktQueue);
                            waitedSeqNum += queue_payload_write(pktQueue, fd, waitedSeqNum, &lastTimeStamp);
                            queue_print_seqNum(pktQueue);
                        }
                        else{
                            fprintf(stderr, "receiver : read_write_loop : couldn't push pkt on pktQueue\n");
                            pkt_del(pkt);
                        }
                        pkt_t *ack = pkt_new();
                        if(!ack){
                            fprintf(stderr, "Malloc error in receiver, read_write_loop, creating ack\n");
                            break;
                        }

                        //push the new ack on the queue 'ackQueue'
                        ack->type = PTYPE_ACK;
                        ack->window = MAX_BUFFER_SIZE;
                        ack->timestamp = lastTimeStamp;
                        if(isFinalSeqNum == 1 && waitedSeqNum == finalSeqNum){
                            fprintf(stderr, "Gonna end, pushing ack of end of transmission soon\n");
                            ack->trFlag = 1;
                            waitedSeqNum++;
                        }
                        ack->seqNum = waitedSeqNum;
                        queue_push(ackQueue, ack);
                    }
                    else{
                        //uint8_t stockValue = (waitedSeqNum+realWindowSize-1) - pkt->seqNum;

                        pkt_t *ack = pkt_new();
                        if(!ack){
                            fprintf(stderr, "Receiver : read_write_loop : creating ack, malloc error\n");
                            pkt_del(pkt);
                            break;
                        }
                        ack->type = PTYPE_ACK;
                        ack->window = MAX_BUFFER_SIZE;
                        ack->seqNum = waitedSeqNum;
                        ack->timestamp = pkt->timestamp;
                        pkt_del(pkt);
                        queue_push(ackQueue, ack);
                    }
                }
            }
        }
    }
    if(pktQueue->size != 0 || ackQueue->size != 0){
		fprintf(stderr, "Error : receiver.c, read_write_loop : gonna return and at least one of the queue is not empty !!!\n");
	}
	queue_free(pktQueue);
	queue_free(ackQueue);
}

int main(int argc, char *argv[]){

    int fd = STDOUT_FILENO; //file from command line
    char *res_hostname; //hostname from command line
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
                //if file doesn't exist : create it, otherwise, reset it
                fd = open(optarg, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
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
        res_hostname = argv[1];
        src_port = atoi(argv[2]);
    }
    else{
        res_hostname = argv[3];
        src_port = atoi(argv[4]);
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
        fprintf(stderr,"Could not connect the socket after the first message.\n");
        close(socket_fd);
        return EXIT_FAILURE;
    }
    
    tpGlobal = malloc(sizeof(struct timespec));
    clock_gettime(CLOCK_REALTIME, tpGlobal);

    read_write_loop(socket_fd, fd);


    free(tpGlobal);
    close(socket_fd);
    close(fd);
    return EXIT_SUCCESS;
}
