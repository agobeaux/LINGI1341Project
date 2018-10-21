#include "queue_sender.h"
#include "Q4 INGInious/packet_interface.h"
#include <time.h>
/**
 * Adds a pkt to the queue.
 *
 * @queue : the queue on which we have to push a node containing pkt
 * @pkt : the content of the new node to push on the queue
 *
 * @return : 0 if the pkt if successful, -1 otherwise
 */
int queue_push(queue_t *queue, pkt_t *pkt, struct timespec *tp){
	fprintf(stderr, "je vais push.c\n");
	fprintf(stderr, "je push seqNum %d\n", pkt->seqNum);

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
		fprintf(stderr, "je vais push in queue with 0 size.c\n");
        newnode->pkt = pkt;
        newnode->tp = tp;
        queue->last = newnode;
        fprintf(stderr, "je vais push last %d\n", newnode->pkt->seqNum);
        queue->head = newnode;
        newnode->next = NULL;
        fprintf(stderr, "je vais push head %d\n", newnode->pkt->seqNum);
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
	fprintf(stderr, "je vais pop.c\n");
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
int queue_delete(queue_t *queue, int seqNum){
	fprintf(stderr, "je vais delete.c\n");
	int number = 0;
	
	fprintf(stderr, "suis dans delete avec seqNum %d\n", seqNum);

    if(queue->size == 0){
        fprintf(stderr, "head NULL, pop in queue.c\n");
        return number;
    }
    
    struct node *run = queue->head;

    while (run != NULL)
    {
		fprintf(stderr, "suis dans delete avec seqNum %d et je run %d\n", seqNum, run->pkt->seqNum);
		if(run->pkt->seqNum!=seqNum){
			fprintf(stderr, "je supprime %d et %d\n", seqNum, run->pkt->seqNum);
			run = run->next;
            queue_pop(queue);
            number=number+1;
            }
        else if(run->pkt->seqNum==seqNum){
			fprintf(stderr, "je supprime %d et %d\n", seqNum, run->pkt->seqNum);
			run = run->next;
            queue_pop(queue);
            number=number+1;
            return number;
            }
    }
    return number;
}


/**
 * Reset the timer.
 *
 * @head : the head of the queue
 *
 * @return return the structure with seqnum
 */
struct node *queue_find_nack_structure(queue_t *queue, int seqNum){
    printf("========= seqNum : %d ============\n", seqNum);
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
 * Initialises an empty queue
 *
 * @return pointer to the newly allocated queue (head node) if successful, NULL otherwise.
 */
queue_t* queue_init(){
	fprintf(stderr, "i'm in init \n");
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
