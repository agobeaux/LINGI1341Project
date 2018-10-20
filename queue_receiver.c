#include "queue_receiver.h"
#include "Q4 INGInious/packet_interface.h"
#include <stdlib.h>
#include <unistd.h>
/**
 * Adds a pkt to the queue.
 *
 * @queue : the queue on which we have to push a node containing pkt
 * @pkt : the content of the new node to push on the queue
 *
 * @return : 0 if the pkt if successful, -1 otherwise
 */
int queue_push(queue_t *queue, pkt_t *pkt){

    if (pkt == NULL){
        fprintf(stderr, "Error, NULL pkt in push, queue. \n");
        return -1;
    }
    node_t *newnode = malloc(sizeof(struct node));
    if (newnode == NULL){
        fprintf(stderr, "Error with malloc in push, queue. \n");
        return -1;
    }

    if(queue->size == 0){
        newnode->pkt = pkt;
        newnode->next = NULL;
        queue->head = newnode;
        queue->size += 1;
        return 0;
    }

    node_t *runner = queue->head;
    while(runner->next != NULL){
        runner = runner->next;
    }
    newnode->pkt = pkt;
    newnode->next = runner->next;
    runner->next = newnode;
    queue->size += 1;
    return 0;
}

/**
 * Adds a pkt to the queue, ordered by seqNum
 *
 * @queue : the queue on which we have to push a node containing pkt
 * @pkt : the content of the new node to push on the queue
 *
 * @return : 0 if the pkt if successful, -1 otherwise
 */
int queue_ordered_push(queue_t *queue, pkt_t *pkt){

    if (pkt == NULL){
        fprintf(stderr, "Error : NULL pkt in queue_ordered_push. \n");
        return -1;
    }
    node_t *newnode = malloc(sizeof(struct node));
    if (newnode == NULL){
        fprintf(stderr, "Error with malloc in queue_ordered_push. \n");
        return -1;
    }

    if(queue->size == 0){
        newnode->pkt = pkt;
        newnode->next = NULL;
        queue->head = newnode;
        queue->size += 1;
        return 0;
    }
    node_t *before = NULL;
    node_t *runner = queue->head;
    while((runner->pkt)->seqNum < pkt->seqNum && runner->next != NULL){
        before = runner;
        runner = runner->next;
    }
    newnode->pkt = pkt;
    if(runner->pkt->seqNum > pkt->seqNum){
        newnode->next = runner;
        if(before == NULL){ // queue->size = 1;
            queue->head = newnode->next;
        }
        else{
            before->next = newnode;
        }
        queue->size += 1;
        return 0;
    }
    else if(runner->pkt->seqNum == pkt->seqNum){
        // pkt already in queue
        free(newnode);
        return -1;
    }
    else{
        // runner->next == NULL, runner->pkt->seqNum > pkt->seqNum
        runner->next = newnode;
        newnode->next = NULL;
        queue->size += 1;
        return 0;
    }
}


/**
 * Removes and returns a pkt from the queue.
 *
 * @head : the head of the queue
 *
 * @return the most recently added pkt on the queue, NULL if queue is empty
 */
pkt_t *queue_pop(queue_t *queue){
    if(queue->size == 0){
        fprintf(stderr, "head NULL, pop in queue.c\n");
        return NULL;
    }
    pkt_t *pkt = queue->head->pkt;
    node_t *save = queue->head;
    queue->head = queue->head->next;
    free(save);
    queue->size -= 1;
    return pkt;
}

/**
 * Initialises an empty queue
 *
 * @return pointer to the newly allocated queue (head node) if successful, NULL otherwise.
 */
queue_t* queue_init(){
    queue_t *newqueue = calloc(1,sizeof(queue_t)); // next and size set to 0
    if(newqueue == NULL){
        fprintf(stderr, "Error : queue_init malloc. \n");
        return NULL;
    }
    return newqueue;
}

/**
 * @return 0 if the queue is empty, 1 otherwise
 */
int queue_isempty(queue_t *queue){
    return queue->size = 0;
}

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
uint8_t queue_payload_write(queue_t *queue, int fd, uint8_t seqNum){
    node_t *runner = queue->head;
    uint8_t count = 0;
    while(runner != NULL && runner->pkt->seqNum == seqNum+count){
        // seqNum+count is of type uint8_t so it will do %(2^8). 254->255->0
        write(fd, runner->pkt->payload, runner->pkt->length);
        count++;
        node_t *toDel = runner;
        runner = runner->next;
        pkt_del(runner->pkt);
        free(runner);
        queue->size -= 1;
    }
    queue->head = runner;
    return count;
}
