#include "queue_sender.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
/**
 * Adds a pkt to the queue.
 *
 * @queue : the queue on which we have to push a node containing pkt
 * @pkt : the content of the new node to push on the queue
 *
 * @return : 0 if the pkt if successful, -1 otherwise
 */
int queue_push(queue_t *queue, pkt_t *pkt, struct timespec *tp){
    fprintf(stderr, "QUEUE SENDER PUSH : tp->tv_sec : %ld, tp->tv_nsec/1000000000 : %ld\n", tp->tv_sec, tp->tv_nsec/1000000000);
    if (pkt == NULL){
        fprintf(stderr, "Error, NULL pkt in push, queue. \n");
        return -1;
    }
    node_t *newnode = malloc(sizeof(struct node));
    if (newnode == NULL){
        fprintf(stderr, "Error with malloc in push, queue. \n");
        pkt_del(pkt);
        return -1;
    }

    if(queue->size == 0){
        newnode->pkt = pkt;
        newnode->tp = tp;
        queue->last = newnode;
        queue->head = newnode;
        newnode->next = NULL;
        queue->size += 1;
        return 0;
    }

    newnode->pkt = pkt;
    newnode->tp = tp;
    queue->last->next = newnode;
    queue->last = newnode;
    queue->last->next = NULL;
    queue->size += 1;
    return 0;
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
    free(save->pkt);
    free(save->tp);
    free(save);
    queue->size -= 1;
    return pkt;
}

/**
 * Removes and returns a pkt from the queue.
 *
 * @head : the number of deleted packets
 *
 * @return delete the pkt with the seqNum on the queue, NULL if queue is empty
 */
int queue_delete(queue_t *queue, uint8_t seqNum){
	int number = 0;

    if(queue->size == 0){
        fprintf(stderr, "head NULL, pop in queue.c\n");
        return number;
    }

    struct node *run = queue->head;

    while (run != NULL)
    {
		if(run->pkt->seqNum!=seqNum){
			run = run->next;
            pkt_t *pktDelete = queue_pop(queue);
            if(pktDelete==NULL){
				fprintf(stderr, "problem in queue_delete\n");
			}
			else{
				pkt_del(pktDelete);
			}
            number=number+1;
        }
        else{ // if(run->pkt->seqNum==seqNum){
			run = run->next;
            pkt_t *pktDelete = queue_pop(queue);
            if(pktDelete==NULL){
				fprintf(stderr, "problem in queue_delete\n");
			}
			else{
				pkt_del(pktDelete);
			}
            number=number+1;
            fprintf(stderr, "queue_delete : just before return, number = %d\n", number);
            if(queue->head == NULL){
                fprintf(stderr, "queue_delete : queue->head = NULL !!\n");
            }
            else{
                fprintf(stderr, "queue_delete : queue->head->pkt->seqNum : %u\n",((queue->head)->pkt)->seqNum);
            }
            return number;
        }
    }
    fprintf(stderr, "queue_delete : run == NULL !!!!\n");
    return number;
}


/**
 * Reset the timer.
 *
 * @head : the head of the queue
 *
 * @return return the structure with seqnum
 */
struct node *queue_find_nack_structure(queue_t *queue, uint8_t seqNum){
    printf("========= seqNum : %u ============\n", seqNum);
    struct node *run = queue->head;
    if(run == NULL){
        printf("queue->head = NULL ! queue_find_nack_structure\n");
    }
    while (run != NULL)
    {
        printf("run->pkt\n");
        pkt_print(run->pkt);
        if (run->pkt->seqNum==seqNum)
        {
            run->tp->tv_sec = 0;
            run->tp->tv_nsec = 0;
            return run;
        }
        run = run->next;
    }
    return NULL;
}

/**
 *
 * @queue : the queue
 *
 * @return return the structure with seqnum
 */
struct node *queue_find_ack_structure(queue_t *queue, uint8_t seqNum){
    printf("========= seqNum : %u ============\n", seqNum);
    struct node *run = queue->head;
    if(run == NULL){
        printf("queue->head = NULL ! queue_find_ack_structure\n");
    }
    while (run != NULL){
        printf("run->pkt\n");
        pkt_print(run->pkt);
        if (run->pkt->seqNum==seqNum){
            return run;
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

void queue_print_seqNum(queue_t *queue){
    node_t *runner = queue->head;
    fprintf(stderr, "Printing queue seqNums :\n");
    while(runner != NULL){
        fprintf(stderr, "%u ",runner->pkt->seqNum);
        runner = runner->next;
    }
    fprintf(stderr, "\n");
}

//TODO : delete ?
void queue_print_first_last_seqNum(queue_t *queue){
    fprintf(stderr, "queue_print_first_last_seqNum : ");
    if(queue->head){
        fprintf(stderr, "queue->head->seqNum : %u, queue->last->seqNum : %u\n", ((queue->head)->pkt)->seqNum, ((queue->last)->pkt)->seqNum);
    }
    else{
        fprintf(stderr, "queue doesn't have first/last...\n");
    }
}
