#ifndef queue_h
#define queue_h



#endif /* queue_h */


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
