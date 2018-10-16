#ifndef stack_h
#define stack_h

#include <stdio.h>

#endif /* stack_h */


/*buffer to stock all structures ready to send*/
typedef struct node {
    pkt_t *pkt;
    struct node *next;
} node;
