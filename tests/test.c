#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

// Do I have to include everything ?? Don't think so

#include "../src/create_socket.h"
#include "../src/packet_interface.h"
//#include "../src/queue_receiver.h"
#include "../src/queue_sender.h"
#include "../src/real_address.h"
//#include "../src/receiver.h"
#include "../src/sender.h"
#include "../src/wait_for_client.h"
#include <time.h>

static uint8_t seqNum = 0;

/**
 * Initialize a new packet.
 *
 * @pkt : packet to initialize
 * @payload : payload to add to the packet
 * @len : the payload's length
 *
 * @return : pkt if the payload is succesfully added onto the pkt, Null otherwise.
 */
pkt_t *create_packet(char *payload, pkt_t *pkt, int len){

    pkt_set_type(pkt, PTYPE_DATA);
    pkt_set_tr(pkt, 0);

    struct timespec *timePkt = malloc(sizeof(struct timespec));
    if(!timePkt){
        fprintf(stderr, "sender : create_packet : couldn't malloc tpGlobal\n");
        return NULL;
    }
    clock_gettime(CLOCK_REALTIME, timePkt);

    pkt_set_timestamp(pkt, timePkt->tv_sec);
    free(timePkt);

    // in our code, the size of the window of the sender is always 31.
    if(pkt_set_window(pkt, 31)!=PKT_OK){
        fprintf(stderr, "sender : create_packet : error with window\n");
        return NULL;
    }
    pkt_set_seqnum(pkt, seqNum++); // does %(2^8) since type(seqNum) : uint8_t

    if(pkt_set_length(pkt, len)!=PKT_OK){
        fprintf(stderr, "sender : create_packet : error with length\n");
        return NULL;
    }
    if(pkt_set_payload(pkt, payload, pkt->length)!=PKT_OK){
        fprintf(stderr, "sender : create_packet : error with payload\n");
        return NULL;
    }
    return pkt;
}

void test_queue_sender(void){
	pkt_t *pkt1 = pkt_new();
	pkt_t *pkt2 = pkt_new();
	pkt_t *pkt3 = pkt_new();
	pkt_t *pkt4 = pkt_new();

	char *payload=(char *)malloc(512);

	pkt1 = create_packet(payload, pkt1, 512);
    pkt2 = create_packet(payload, pkt2, 512);
    pkt3 = create_packet(payload, pkt3, 512);
    pkt4 = create_packet(payload, pkt4, 512);

	queue_t *buf_structure = queue_init();
	pkt_set_seqnum (pkt4, 254);
    queue_push(buf_structure, pkt4);
    pkt_set_seqnum (pkt2, 255);
    queue_push(buf_structure, pkt2);
    pkt_set_seqnum (pkt1, 0);
    queue_push(buf_structure, pkt1);
    pkt_set_seqnum (pkt3, 1);
    queue_push(buf_structure, pkt3);

	int nbNodes = 0;
	node_t *run = buf_structure->head;
	while(run != NULL){
		uint8_t expected_seqNum = 254+nbNodes;
		CU_ASSERT_EQUAL(run->pkt->seqNum, expected_seqNum);
		nbNodes++;
		run = run->next;
	}
	CU_ASSERT_EQUAL(nbNodes, 4);
	CU_ASSERT_EQUAL(buf_structure->size, 4);
	int nbDel = queue_delete(buf_structure, 0);
	CU_ASSERT_EQUAL(nbDel, 3);
	CU_ASSERT_EQUAL(buf_structure->head->pkt->seqNum, 1);
	CU_ASSERT_PTR_NOT_NULL(buf_structure->head);
	CU_ASSERT_EQUAL(buf_structure->head->pkt->seqNum, 1);
	CU_ASSERT_PTR_NULL(buf_structure->head->next);
	CU_ASSERT(buf_structure->head == buf_structure->last);
	free(payload);
	pkt_del(buf_structure->head->pkt);
	free(buf_structure->head);
	free(buf_structure);
}

void test_implem_decode_encode(void){

	pkt_t *pkt = pkt_new();
	pkt_set_type(pkt, PTYPE_ACK);
	pkt_set_tr(pkt, 0);
	pkt_set_window(pkt, 31);
	pkt_set_seqnum(pkt, 0);
	char *payload = "Hello world!";
	pkt_set_length(pkt, strlen(payload)+1); // +1 for '\0'
	pkt_set_payload(pkt, payload, strlen(payload)+1);
	pkt_set_timestamp(pkt, 0);

	size_t len = 16 + strlen(payload)+1;
	char *buf = malloc(len);

	pkt_status_code status = pkt_encode(pkt, buf, &len);
	CU_ASSERT_EQUAL(status, PKT_OK);
	CU_ASSERT_EQUAL(len, 16 + strlen(payload)+1);

	pkt_t *pkt2 = pkt_new();
	status = pkt_decode(buf, len, pkt2);
	CU_ASSERT_EQUAL(status, PKT_OK);
	CU_ASSERT_EQUAL(memcmp(pkt, pkt2, 12), 0); // Header comparison
	CU_ASSERT_STRING_EQUAL(pkt2->payload, pkt->payload);
	CU_ASSERT_EQUAL(pkt2->crc2, pkt->crc2);
	free(buf);
	pkt_del(pkt);
	pkt_del(pkt2);
}

int main(){
	// initialization of the registry
	if(CU_initialize_registry() != CUE_SUCCESS){
		fprintf(stderr, "%s\n", CU_get_error_msg()); // good ?
		return CU_get_error();
	}

	// initialization of the suite
	CU_pSuite suite = CU_add_suite("suite_interface", NULL, NULL);

	if(suite == NULL){
		CU_cleanup_registry();
		fprintf(stderr, "%s\n", CU_get_error_msg()); // good ?
		return CU_get_error();
	}

	if (CU_add_test(suite, "Test decode followed by encode", test_implem_decode_encode) == NULL ||
		CU_add_test(suite, "Test of queue as sender", test_queue_sender) == NULL) {
		CU_cleanup_registry();
		fprintf(stderr, "%s\n", CU_get_error_msg()); // good ?
		return CU_get_error();
	}

	CU_basic_run_tests();

	CU_basic_show_failures(CU_get_failure_list());

	CU_cleanup_registry();

	return CU_get_error();


}
