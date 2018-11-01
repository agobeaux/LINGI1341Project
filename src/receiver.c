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
 *Loop for writing packets and reading the acks/nacks
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
    fprintf(stderr, "finalSeqNum = -1 -> %u\n",finalSeqNum);
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
			return;
		}

		free(tpNow);

        if(poll(pfds, 1, 5) == -1){
            fprintf(stderr, "Error poll call : %s\n", strerror(errno));
        }

        //fprintf(stderr, "pfds[0].revents&POLLOUT : %d, &POLLIN : %d\n", pfds[0].revents&POLLOUT, pfds[0].revents&POLLIN);
        while(pfds[0].revents&POLLOUT && ackQueue->size > 0){ // write on socket
            fprintf(stderr, "size of pkt queue : %d, ack queue : %d\n", pktQueue->size, ackQueue->size);
            fprintf(stderr, "Writing on the socket\n");
            pkt_t *ack = queue_pop(ackQueue);
            if(ack == NULL){
                fprintf(stderr, "receiver : read_write_loop, ack == NULL when queue_pop(ackQueue)\n");
                continue; //TODO : continue or break ? ack shouldn't be null if queue isn't empty
            }
            int trFlag = ack->trFlag;
            ack->trFlag = 0;
            char ackBuf[12];
            size_t ackLen = 12;
            if(pkt_encode(ack, ackBuf, &ackLen) != PKT_OK){
                pkt_del(ack);
                fprintf(stderr, "encode error in receiver, read_write_loop, encode != PKT_OK\n");
            }
            else if(ackLen != 12){
				pkt_del(ack);
                fprintf(stderr, "receiver : read_write_loop, ackLen != 12. ack not send.\n");
            }
            else{
                fprintf(stderr, "I'm really writing\n");
                int wr = write(sfd, ackBuf, ackLen);
                fprintf(stderr, "Wrote %d bytes. Seqnum sent : %u\n", wr, ack->seqNum);
                pkt_del(ack);
                if(wr == -1){
                    fprintf(stderr, "code : %d, %s\n", errno, gai_strerror(errno));
                }
                if(trFlag == 1){
                    fprintf(stderr, "trFlag == 1\n");
                    if(pktQueue->size != 0 || ackQueue->size != 0){
						fprintf(stderr, "Error : receiver.c, read_write_loop : gonna return and at least one of the queue is not empty !!!\n");
					}
                    free(pktQueue);
                    free(ackQueue);
                    return;
                }
            }
        }

        if(pfds[0].revents&POLLIN){

			//reset the transmission timer because there is still something to read
			clock_gettime(CLOCK_REALTIME, tpGlobal);

            fprintf(stderr,"=============== HERE ==============\n");
            fprintf(stderr, "size of pkt queue : %d, ack queue : %d\n", pktQueue->size, ackQueue->size);
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
                //TODO : put this uint somewhere else, replace it with a #define
                uint8_t realWindowSize = 31;
                pkt_t *pkt = pkt_new();
                if(!pkt){
                    fprintf(stderr, "Malloc error in receiver, read_write_loop, creating pkt\n");
                    break;
                }
                pkt_status_code code = pkt_decode(buf, rd, pkt);
                if(code != PKT_OK){

                    pkt_t *ack = pkt_new();
                    if(!ack){
                        fprintf(stderr, "Receiver : read_write_loop : creating ack, malloc error\n");
                        break;
                    }
                    ack->type = PTYPE_ACK;
                    ack->window = 31;
                    ack->seqNum = waitedSeqNum;
                    ack->timestamp = 0;
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
                    nack->window = 31;
                    nack->seqNum = pkt->seqNum;
                    nack->timestamp = pkt->timestamp;
                    pkt_del(pkt);
                    queue_push(ackQueue, nack);
                }
                else{
                    //here, check if pkt is included in the window.
                    // Useful variables to know if the pk(s seqNum lies in the window or not
                    // uint8_t so that 1-2 = 255 for example.
                    uint8_t distInf = pkt->seqNum - waitedSeqNum; // distance between the beginning of the window and the seqNum
                    uint8_t distSup = (waitedSeqNum+realWindowSize-1) - pkt->seqNum; // distance between the end of the window and the seqNum
                    if(distInf < realWindowSize && distSup < realWindowSize){
                        fprintf(stderr, "packet in window size, receiver.c, seqnum = %u, waited : %u\n", pkt->seqNum, waitedSeqNum);
                        // packet received
                        if(pkt->length == 0){
                            //end of transmission packet
                            fprintf(stderr, "End of transmission packet, pkt->length = 0\n");
                            pkt_t *ack = pkt_new();
                            if(!ack){
                                fprintf(stderr, "Malloc error in receiver, read_write_loop, creating ack\n");
                                pkt_del(pkt);
                                break;
                            }
                            ack->type = PTYPE_ACK;
                            isFinalSeqNum = 1;
                            finalSeqNum = pkt->seqNum;
                            if(pkt->seqNum == waitedSeqNum){
                                ack->trFlag = 1; //TODO : I defined this to know which ack should be the last one
                                waitedSeqNum++;
                            }
                            ack->window = 31;
                            ack->seqNum = waitedSeqNum;
                            ack->timestamp = pkt->timestamp;
                            queue_push(ackQueue, ack);
                            fprintf(stderr, "before continue\n");
                            pkt_del(pkt);
                            continue;
                        }
                        if(queue_ordered_push(pktQueue, pkt, waitedSeqNum, realWindowSize) == 0){
                            // pkt successfully added on the queue
                            queue_print_seqNum(pktQueue);
                            waitedSeqNum += queue_payload_write(pktQueue, fd, waitedSeqNum, &lastTimeStamp);
                            queue_print_seqNum(pktQueue);
                        }
                        else{
                            fprintf(stderr, "receiver : read_write_loop : couldn't push pkt on pktQueue\n");
                            //TODO : WARNING : it sends the ack with timestamp but the packet isn't stocked in this else...
                            pkt_del(pkt);
                        }
                        pkt_t *ack = pkt_new();
                        if(!ack){
                            fprintf(stderr, "Malloc error in receiver, read_write_loop, creating ack\n");
                            break;
                        }
                        ack->type = PTYPE_ACK;
                        ack->window = 31;
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
                        fprintf(stderr, "Receiver : read_write_loop : received pkt outside of the window\n");
                        fprintf(stderr, "pkt->seqNum : %u, waitedSeqNum : %u, realWindowSize : %u\n", pkt->seqNum, waitedSeqNum, realWindowSize);
                        uint8_t stockValue = (waitedSeqNum+realWindowSize-1) - pkt->seqNum;
                        fprintf(stderr, "(waitedSeqNum+realWindowSize-1) - pkt->seqNum : %u\n",(waitedSeqNum+realWindowSize-1) - pkt->seqNum);
                        fprintf(stderr, "stockValue : %u\n", stockValue);
                        fprintf(stderr, "pkt->seqNum - waitedSeqNum : %u\n", pkt->seqNum - waitedSeqNum);
                        fprintf(stderr, "pkt->seqNum - waitedSeqNum < realWindowSize : %u ;(waitedSeqNum+realWindowSize-1) - pkt->seqNum < realWindowSize : %u\n", pkt->seqNum - waitedSeqNum < realWindowSize, (waitedSeqNum+realWindowSize-1) - pkt->seqNum < realWindowSize);

                        pkt_t *ack = pkt_new();
                        if(!ack){
                            fprintf(stderr, "Receiver : read_write_loop : creating ack, malloc error\n");
                            pkt_del(pkt);
                            break;
                        }
                        ack->type = PTYPE_ACK;
                        ack->window = 31;
                        ack->seqNum = waitedSeqNum;
                        ack->timestamp = pkt->timestamp;
                        pkt_del(pkt);
                        queue_push(ackQueue, ack);
                    }
                }
            }
        } // end of if(pfds[0].revents&POLLIN)
    } // end of while(1)
    if(pktQueue->size != 0 || ackQueue->size != 0){
		fprintf(stderr, "Error : receiver.c, read_write_loop : gonna return and at least one of the queue is not empty !!!\n");
	}
	free(pktQueue);
	free(ackQueue);
}

//TODO : ne pas oublier : taille de fenêtre 2^(n-1) au max.
// comment la taille de fenêtre varie ? Dépend de nous ? sert à quoi ?
int main(int argc, char *argv[]){

    int fd = STDOUT_FILENO; //file from command line
    char *res_hostname; //hostname from command line
    int src_port; //port from command line
    int socket_fd; //socket file descriptor

    tpGlobal = malloc(sizeof(struct timespec));
    clock_gettime(CLOCK_REALTIME, tpGlobal);



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

    fprintf(stderr, "Before read_write_loop in main\n");
    read_write_loop(socket_fd, fd);


    free(tpGlobal);
    close(socket_fd);
    close(fd);
    return EXIT_SUCCESS;
}
