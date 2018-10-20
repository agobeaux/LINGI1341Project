#include "queue.h"
#include "Q4 INGInious/packet_interface.h" /* for pkt_del */

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

    node_t *run = queue->head;
    while(run->next != NULL){
        run = run->next;
    }
    newnode->pkt = pkt;
    newnode->next = run->next;
    run->next = newnode;
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
    node_t *run = queue->head;
    while(run->pkt->seqNum < pkt->seqNum && run->next != NULL){
        before = run;
        run = run->next;
    }
    newnode->pkt = pkt;
    if(run->pkt->seqNum > pkt->seqNum){
        newnode->next = run;
        if(before == NULL){ // queue->size = 1;
            queue->head = newnode->next;
        }
        else{
            before->next = newnode;
        }
        queue->size += 1;
        return 0;
    }
    else if(run->pkt->seqNum == pkt->seqNum){
        // pkt already in queue
        free(newnode);
        return -1;
    }
    else{
        // run->next == NULL, run->pkt->seqNum > pkt->seqNum
        run->next = newnode;
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
 * Removes and returns a pkt from the queue.
 *
 * @head : the head of the queue
 *
 * @return delete the pkt with the seqNum on the queue, NULL if queue is empty
 */
pkt_t *queue_delete(queue_t *queue, int seqNum){

    if(queue->size == 0){
        fprintf(stderr, "head NULL, pop in queue.c\n");
        return NULL;
    }

    struct node *run = queue->head;
    if(run->pkt->seqNum == seqNum){
        return queue_pop(queue);
    }

    while (run->next != NULL)
    {
        if (run->next->pkt->seqNum==seqNum)
        {
            pkt_t *pkt = run->next->pkt;
            node_t *save = run->next;
            run = run->next->next;
            free(save);
            queue->size -= 1;
            return pkt;
        }
        run = run->next;
    }
    return NULL;
}

/**
 * Removes and returns a pkt from the queue.
 *
 * @head : the head of the queue
 *
 * @return return the structure with seqnum
 */
pkt_t *queue_find_nack_structure(queue_t *queue, int seqNum){
    struct node *run = queue->head;

    while (run->next != NULL)
    {
        if (run->pkt->seqNum==seqNum)
        {
            return run->pkt;
        }
        run = run->next;
    }
    return NULL;
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
