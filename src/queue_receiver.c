#include "queue_receiver.h"
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
int queue_ordered_push(queue_t *queue, pkt_t *pkt, uint8_t waitedSeqNum, uint8_t realWindowSize){

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
    if(waitedSeqNum >= 255 - (realWindowSize-1) && pkt->seqNum < realWindowSize-1){
        // we have something like 245.. 255 0 ... 3.
        // the pkt to insert has a seqNum that is in [0 ,realWindowSize-1[
        while(runner != NULL && (runner->pkt)->seqNum >= 255 - (realWindowSize-1)){
            before = runner;
            runner = runner->next;
        }
        if(runner == NULL){
            // here : no numbers in [0, realWindowSize-1[ in the queue. Just put it next then
            newnode->next = NULL;
            newnode->pkt = pkt;
            before->next = newnode;
            queue->size += 1;
            return 0;
        }
        // here : now we can loop to know where we have to put the right number node
        // because now, nodes after runner are contained in [0, realWindowSize-1[
    }
    if(waitedSeqNum >= 255 - (realWindowSize-1) && pkt->seqNum >= 255 - (realWindowSize-1)){
        while(runner != NULL && (runner->pkt)->seqNum >= 255 - (realWindowSize-1) && (runner->pkt)->seqNum < pkt->seqNum){
            before = runner;
            runner = runner->next;
        }
        if(runner == NULL){
            newnode->pkt = pkt;
            before->next = newnode;
            newnode->next = NULL;
        }
        else{
            // 1st case : !((runner->pkt)->seqNum >= 255 - (realWindowSize-1))
            // node runner is in [0, realWindowSize-1[
            // 2nd case : !((runner->pkt)->seqNum < pkt->seqNum)
            // node runner is greater than or equal to newnode (in terms of seqNum)
            if(before == NULL){
                queue->head = newnode;
            }
            else{
                before->next = newnode;
            }
            newnode->pkt = pkt;
            newnode->next = runner;
        }
        queue->size += 1;
        return 0;
    }
    else{
        while((runner->pkt)->seqNum < pkt->seqNum && runner->next != NULL){
            before = runner;
            runner = runner->next;
        }
        newnode->pkt = pkt;
        if(runner->pkt->seqNum > pkt->seqNum){
            newnode->next = runner;
            if(before == NULL){ // queue->size = 1;
                queue->head = newnode;
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
    pkt_del(save->pkt);
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
    fprintf(stderr, "queue_rec, payload write, size of queue in the beginning : %d\n", queue->size);
    uint8_t newSeqNum = seqNum;
    while(runner != NULL && runner->pkt->seqNum == newSeqNum){
        // seqNum+count is of type uint8_t so it will do %(2^8). 254->255->0
        int wr = write(fd, runner->pkt->payload, runner->pkt->length);
        if(wr == -1){
            fprintf(stderr, "queue_receiver : queue_payload_write : write = -1\n");
        }
        count++;
        node_t *toDel = runner;
        runner = runner->next;
        pkt_del(toDel->pkt);
        free(toDel);
        queue->size -= 1;
        newSeqNum++;
    }
    queue->head = runner;
    fprintf(stderr, "queue_receiver.c, write payload returns : %u\n",count);
    return count;
}

void queue_print_seqNum(queue_t *queue){
    node_t *runner = queue->head;
    fprintf(stderr, "Printing queue seqNums :\n");
    while(runner != NULL){
        fprintf(stderr, "%u ",runner->pkt->seqNum);
        runner = runner->next;
    }
    fprintf(stderr, "\n");
}
