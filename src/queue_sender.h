#ifndef queue_sender_h
#define queue_sender_h

#include "packet_interface.h"

/*buffer to stock all structures ready to send*/
typedef struct node {
    pkt_t *pkt;
    struct node *next;
    struct timespec *tp;
} node_t;

typedef struct queue{
    struct node *head;
    struct node *last;
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
int queue_push(queue_t *queue, pkt_t *pkt, struct timespec *tp);

/**
 * Removes and returns a pkt from the queue.
 *
 * @head : the head of the queue
 *
 * @return the most recently added pkt on the queue, NULL if queue is empty
 */
pkt_t *queue_pop(queue_t *queue);

/**
 * Removes and returns a pkt from the queue.
 *
 * @head : the head of the queue
 *
 * @return delete the pkt with the seqNum on the queue, NULL if queue is empty
 */
int queue_delete(queue_t *queue, uint8_t seqNum);

struct node *queue_find_ack_structure(queue_t *queue, uint8_t seqNum);

/**
 * Reset the timer.
 *
 * @head : the head of the queue
 *
 * @return return the structure with seqnum
 */
struct node *queue_find_nack_structure(queue_t *queue, uint8_t seqNum);

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

void queue_print_seqNum(queue_t *queue);

// TODO delete ?
void queue_print_first_last_seqNum(queue_t *queue);

#endif /* queue_sender_h */
