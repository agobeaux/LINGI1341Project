#ifndef queue_sender_h
#define queue_sender_h

#include "packet_interface.h"

/*node structure*/
typedef struct node {
    pkt_t *pkt;
    struct node *next;
} node_t;

/*queue to stock all structures ready to send*/
typedef struct queue{
    struct node *head;
    struct node *last;
    int size;
} queue_t;

/**
 * Adds a pkt to the queue.
 *
 * @queue : the queue on which we have to push a node containing pkt
 * @pkt : the packet of the new node to push on the queue
 *
 * @return : 0 if the push if successful, -1 otherwise
 */
int queue_push(queue_t *queue, pkt_t *pkt);

/**
 * Removes and returns the first packet from the queue.
 *
 * @queue : the queue from which we have to pop the first node
 *
 * @return : the first added pkt on the queue, NULL if queue is empty
 */
pkt_t *queue_pop(queue_t *queue);

/**
 * Deletes all packets from pkt with the seqnum <= seqNum
 *
 * @queue : the queue from which we have to delete the packet with seqNum
 * @seqNum : the seqnum of the packet
 *
 * @return : number of deleted packets
 */
int queue_delete(queue_t *queue, uint8_t seqNum);

/**
 * Finds and returns the packet from pkt with the seqNum and also reset the time
 *
 * @queue : the queue where we try to find the packet
 * @seqNum : the seqNum of the packet
 *
 * @return : return the structure with seqnum
 */
pkt_t *queue_find_nack_structure(queue_t *queue, uint8_t seqNum);

/**
 * Finds and returns the packet from pkt with the seqNum
 *
 * @queue : the queue where we try to find the packet
 * @seqNum : the seqNum of the packet
 *
 * @return : the packet with seqNum, NULL otherwise
 */
pkt_t *queue_find_ack_structure(queue_t *queue, uint8_t seqNum);

/**
 * Initialises an empty queue
 *
 * @return : pointer to the newly allocated queue (head node) if successful, NULL otherwise.
 */
queue_t* queue_init();

/**
 * Checks if queue is empty
 * 
 * @return : 0 if the queue is empty, 1 otherwise
 */
int queue_isempty(queue_t *queue);

/**
 * Prints all queue's seqNum
 * 
 * @queue : the queue to print
 */
void queue_print_seqNum(queue_t *queue);

/**
 * Prints the first and the last queue's seqNum
 * 
 * @queue : the queue to print
 */
void queue_print_first_last_seqNum(queue_t *queue);

/**
 * Frees queue
 * 
 * @queue : the queue to free
 */
void queue_free(queue_t *queue);

#endif /* queue_sender_h */
