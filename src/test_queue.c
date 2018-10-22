#include "test_queue.h"
#include <poll.h>
#include "packet_implem.c"
#include "real_address.c"
#include "create_socket.c"
#include "queue_sender.c"
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

pkt_t *create_packet(char *payload, pkt_t *pkt){
    //set data
    pkt_set_type(pkt, PTYPE_DATA);
    pkt_set_tr(pkt, 0);
    pkt_set_timestamp(pkt, 0);
    pkt_set_window(pkt, 1);
    pkt_set_seqnum(pkt, 0);
    pkt_set_length(pkt, strlen(payload));
    pkt_set_payload(pkt, payload, pkt->length);
    return pkt;
}


int main(int argc, char *argv[]){

    int err;

    size_t len = 528;
    char *buf = (char*)malloc(528);
    char *new_payload=(char *)malloc(MAX_PAYLOAD_SIZE);

    int fd = open(argv[1], O_RDONLY);
    if(fd == -1){
        fprintf(stderr, "Can't open the file!\n");
        exit(EXIT_FAILURE);
    }

    //try to have a new payload
    err = read(fd, (void *)new_payload, MAX_PAYLOAD_SIZE);
    if(err == -1){
        fprintf(stderr, "Failed to read a new payload!\n");
        exit(EXIT_FAILURE);
    }
    if(err == 0){
        fprintf(stderr, "There is no more payloads to read!\n");
    }

    //encode a new structure
    pkt_t* pkt1 = pkt_new();
    pkt_t* pkt2 = pkt_new();
    pkt_t* pkt3 = pkt_new();
    pkt_t* pkt4 = pkt_new();

    pkt1 = create_packet(new_payload, pkt1);
    pkt2 = create_packet(new_payload, pkt2);
    pkt3 = create_packet(new_payload, pkt3);
    pkt4 = create_packet(new_payload, pkt4);
    pkt_encode(pkt1, buf, &len);
    pkt_encode(pkt2, buf, &len);
    pkt_encode(pkt3, buf, &len);
    pkt_encode(pkt4, buf, &len);
    struct timespec *tp = malloc(sizeof(struct timespec));
    clock_gettime(CLOCK_REALTIME, tp);


    queue_t *buf_structure = queue_init();
    if(buf_structure == NULL){
        fprintf(stderr, "Error : test_queue, main, malloc buf_structure");
    }
    pkt_set_seqnum (pkt1, 1);
    queue_push(buf_structure, pkt1, tp);
    pkt_set_seqnum (pkt2, 2);
    queue_push(buf_structure, pkt2, tp);
    pkt_set_seqnum (pkt3, 3);
    queue_push(buf_structure, pkt3, tp);
    pkt_set_seqnum (pkt4, 4);
    queue_push(buf_structure, pkt4, tp);
    fprintf(stderr, "Head of queue has seqNum %d\n", buf_structure->head->pkt->seqNum);
    fprintf(stderr, "Last of queue has seqNum %d\n", buf_structure->last->pkt->seqNum);
    fprintf(stderr, "2nd of queue has seqNum %d\n", buf_structure->head->next->pkt->seqNum);
    fprintf(stderr, "3d of queue has seqNum %d\n", buf_structure->head->next->next->pkt->seqNum);
    fprintf(stderr, "Head's seqNum is  %d\n", buf_structure->head->pkt->seqNum);
    fprintf(stderr, "number of deleted pkt is %d\n", (queue_delete(buf_structure, 3)));
    fprintf(stderr, "Head's seqNum is  %d\n", buf_structure->head->pkt->seqNum);
    pkt_set_seqnum (pkt1, 5);
    queue_push(buf_structure, pkt1, tp);
    pkt_set_seqnum (pkt2, 6);
    queue_push(buf_structure, pkt2, tp);
    fprintf(stderr, "Head of queue has seqNum %d\n", buf_structure->head->pkt->seqNum);
    fprintf(stderr, "Last of queue has seqNum %d\n", buf_structure->last->pkt->seqNum);
    fprintf(stderr, "number of deleted pkt is %d\n", (queue_delete(buf_structure, 5)));
    fprintf(stderr, "Head's seqNum is  %d\n", buf_structure->head->pkt->seqNum);

}
