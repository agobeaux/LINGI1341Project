#include "stack.h"


static node *head = NULL;



/**
 * Adds a pkt to the stack.
 *
 * @param pkt is a structure to be added to the stack.
 *
 * @return If the pkt is succesfully pushed onto the stack, returns 0. If not, the returned value is -1.
 */
int push(pkt_t *pkt)
{
    node *new_node = malloc(sizeof(struct node));
    
    if (new_node == NULL)
    {
        fprintf(stderr, "Error with malloc in push. \n");
        return -1;
    }
    
    if (pkt == NULL)
    {
        fprintf(stderr, "Error, NULL structure in push. \n");
        return -1;
    }
    
    new_node->pkt = pkt;
    
    new_node->next = head;
    head = new_node;
    
    return 0;
}

/**
 * Removes and returns a pkt from the stack.
 *
 * @return the most recently added pkt on the stack.
 */
pkt_t *pop()
{
    pkt_t *pkt = head->pkt;
    node *save = head;
    head = head->next;
    
    free(save);
    return pkt;
}


/**
 * Initialises the stack and the killer nodes.
 *
 * @param size is the size of the stack.
 * @param size_buffer is the maximum number of pkt to send.
 *
 * @return 0 if initialisation succesful, -1 if not.
 */
int init(int size, int size_buffer)
{
    node *new_node = malloc(sizeof(struct node));
    if (new_node == NULL){
        fprintf(stderr, "Error with malloc in init. \n");
        return -1;
    }
    
    int i;
    for (i = 0; i < size_buffer; ++i)
    {
        new_node->pkt = NULL;
        new_node->next = head;
        head = new_node;
    }
    return 0;
}
/**
 * Frees the stack.
 */
void free_stack()
{
    struct node* tmp;
    
    while (head != NULL)
    {
        tmp = head;
        head = head->next;
        pkt_del(tmp->pkt);
        free(tmp);
    }
}
