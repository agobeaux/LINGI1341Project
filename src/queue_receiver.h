#ifndef queue_receiver_h
#define queue_receiver_h

#include "packet_interface.h"
#include <stdio.h>

/*buffer to stock all structures ready to send*/
typedef struct node {
    pkt_t *pkt;
    struct node *next;
} node_t;

typedef struct queue{
    struct node* head;
    int size;
} queue_t;

/**
 * Adds a pkt to the queue.
 *
 * @queue : the queue on which we have to push a node containing pkt
 * @pkt : the content of the new node to push on the queue
 *
 * @return : 0 if the pkt if successful, -1 otherwise
 */
int queue_push(queue_t *queue, pkt_t *pkt);

/**
 * Adds a pkt to the queue, ordered by seqNum
 *
 * @queue : the queue on which we have to push a node containing pkt
 * @pkt : the content of the new node to push on the queue
 *
 * @return : 0 if the pkt if successful, -1 otherwise
 */
int queue_ordered_push(queue_t *queue, pkt_t *pkt, uint8_t waitedSeqNum, uint8_t realWindowSize);


/**
 * Removes and returns a pkt from the queue.
 *
 * @head : the head of the queue
 *
 * @return the most recently added pkt on the queue, NULL if queue is empty
 */
pkt_t *queue_pop(queue_t *queue);

/**
 * Initialises an empty queue
 *
 * @return pointer to the newly allocated queue (head node) if successful, NULL otherwise.
 */
queue_t* queue_init();

/**
 * @return 0 if the queue is empty, 1 otherwise
 */
int queue_isempty(queue_t *queue);

/**
 * Write and delete the packets (from seqNum, write while packets are consecutives
 * to the file (already open)
 *
 * @queue : the ordered queue
 * @fd : the file descriptor of the open file
 * @seqNum : the sequence number that should be the first written packet
 *
 * @return : the number of written packets with type uint8_t so that
 *           waitedSeqNum = lastSeqNum + count; will do %(2^8)
 */
uint8_t queue_payload_write(queue_t *queue, int fd, uint8_t seqNum);

void queue_print_seqNum(queue_t *queue);

#endif /* queue_receiver_h */
