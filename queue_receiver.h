#ifndef queue_receiver_h
#define queue_receiver_h



#endif /* queue_receiver_h */


/*buffer to stock all structures ready to send*/
typedef struct node {
    pkt_t *pkt;
    struct node *next;
} node_t;

typedef struct queue{
    struct node* head;
    int size;
} queue_t;
