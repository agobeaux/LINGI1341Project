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
int queue_push(node_t* head, pkt_t *pkt){
    if(head == NULL){
        fprintf(stderr, "push function called on NULL head (argument), queue\n");
        return -1;
    }
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
    new_node->next = head;
    head = new_node;

    return 0;
}

/**
 * Removes and returns a pkt from the queue.
 *
 * @head : the head of the queue
 *
 * @return the most recently added pkt on the queue, NULL if queue is empty
 */
pkt_t *queue_pop(node_t *head){
    if(head == NULL){
        fprintf(stderr, "head NULL, pop in queue.c\n");
        return NULL;
    }
    pkt_t *pkt = head->pkt;
    node_t *save = head;
    head = head->next;

    free(save); // are we sure that it only frees the save memory ? what about save pkt
    // save->pkt isn't freed but... check
    return pkt;
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
