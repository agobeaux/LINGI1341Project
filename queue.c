#include "queue.h"
#include "Q4 INGInious/packet_interface.h" /* for pkt_del */

/**
 * Adds a pkt to the queue.
 *
 * @head : the head of the queue
 * @pkt : the content of the new node to push on the queue pkt is a structure to be added to the queue
 *
 * @return : 0 if the pkt if successful, -1 otherwise
 */
int queue_push(queue_t *list, pkt_t *pkt){
    
    //if(list->head == NULL){
        //fprintf(stderr, "push function called on NULL head (argument), queue\n");
        //return -1;
    //}
    if (pkt == NULL){
        fprintf(stderr, "Error, NULL pkt in push, queue. \n");
        return -1;
    }
    
    node_t *new_node = malloc(sizeof(struct node));
    if (new_node == NULL){
        fprintf(stderr, "Error with malloc in push, queue. \n");
        return -1;
    }
    
    new_node->pkt = pkt;
    new_node->next = list->head;
    list->head = new_node;

    return 0;
}


/**
 * Removes and returns a pkt from the queue.
 *
 * @head : the head of the queue
 *
 * @return the most recently added pkt on the queue, NULL if queue is empty
 */
pkt_t *queue_pop(queue_t *list){
    if(list->head == NULL){
        fprintf(stderr, "head NULL, pop in queue.c\n");
        return NULL;
    }
    pkt_t *pkt = list->head->pkt;
    node_t *save = list->head;
    list->head = list->head->next;
    fprintf(stderr, "I want delete %s\n", pkt->payload);
    free(save);
    return pkt;
}

/**
 * Removes and returns a pkt from the queue.
 *
 * @head : the head of the queue
 *
 * @return delete the pkt with the seqNum on the queue, NULL if queue is empty
 */
pkt_t *delete(queue_t *list, int seqNum){
    struct node *run = list->head;
    if(run->pkt->seqNum == seqNum){
        return queue_pop(list);
    }
    
    while (run->next != NULL)
    {
        fprintf(stderr, "I'm in %d\n", run->next->pkt->seqNum);
        if (run->next->pkt->seqNum==seqNum)
        {
            pkt_t *pkt = run->next->pkt;
            node_t *save = run->next;
            run = run->next->next;
            fprintf(stderr, "I want delete %s\n", pkt->payload);
            free(save);
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
pkt_t *find_nack_structure(queue_t *list, int seqNum){
    struct node *run = list->head;
    
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
 * Initialises the first node of the queue
 *
 * @pkt the pkt of the first node
 *
 * @return pointer to the newly allocated queue (head node) if successful, NULL otherwise.
 */
node_t* queue_init(pkt_t *pkt){
    node_t *new_node = malloc(sizeof(struct node));
    if (new_node == NULL){
        fprintf(stderr, "Error with malloc in init, queue. \n");
        return NULL;
    }
    new_node->pkt = pkt;
    new_node->next = NULL;

    return new_node;
}

/**
 * @return 0 if the queue is empty, 1 otherwise
 */
int queue_isempty(node_t *head){
    return head != NULL;
}

/**
 * Frees the queue.
 */
void queue_free(node_t *head){
    struct node* tmp;
    while (head != NULL){
        tmp = head;
        head = head->next;
        pkt_del(tmp->pkt);
        free(tmp);
    }
}
