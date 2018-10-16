#include "packet_interface.h"

/* Extra #includes */
/* Your code will be inserted here */
#include <zlib.h> /* for crc32 function */
#include <string.h> /* memcpy */
#include <stdlib.h> /* realloc */
#include <arpa/inet.h> /* ntohs */
#include <stdio.h>

struct __attribute__((__packed__)) pkt {
	// Changed order because of the endianness
	uint8_t window:5;
	uint8_t trFlag:1;
	uint8_t type:2;
	uint8_t seqNum;
	uint16_t length;
	uint32_t timestamp;
	uint32_t crc1;
	char *payload;
	uint32_t crc2;
};

/* Extra code */
/* Your code will be inserted here */

pkt_t* pkt_new()
{
	return calloc(1, sizeof(pkt_t));
}

void pkt_del(pkt_t *pkt)
{
    if(!pkt){
		if(!pkt->payload){
			free(pkt->payload);
		}
		free(pkt);
	}
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
	if(len < 12){
		return E_NOHEADER;
	}
	memcpy(pkt, data, sizeof(uint8_t)); // window trFlag type
	if(pkt->type < 1 || pkt->type > 3){
		free(pkt);
		return E_TYPE;
	}
	if(pkt->trFlag == 1 && pkt->type != PTYPE_DATA){
		free(pkt);
		return E_TR;
	}
	if(pkt->trFlag == 1 && len != 12){
		free(pkt);
		return E_TR;
	}
	// no if for the window
	memcpy((void*)pkt+1, data+1, sizeof(uint8_t)); // seqNum
	// no if for seqNum
	memcpy((void*)pkt+2, data+2, sizeof(uint16_t)); // length
	uint16_t sLength = ntohs(pkt->length);
	if(sLength > MAX_PAYLOAD_SIZE){
		free(pkt);
		return E_LENGTH;
	}
	memcpy((void*)pkt+4, data+4, sizeof(uint32_t)); // pkt_get_timestamp
	memcpy((void*)pkt+8, data+8, sizeof(uint32_t)); // crc1
	pkt->crc1 = ntohl(pkt->crc1);
	uint8_t trTmp = pkt->trFlag;
	pkt->trFlag = 0; // to calculate crc1
	if(crc32(0, (const Bytef*)pkt, 8) != pkt->crc1){ // 8 bytes -> 64 bits
		free(pkt);
		return E_CRC;
	}
	pkt->trFlag = trTmp;
	pkt->length = sLength;

	if(len == 12){
		return PKT_OK;
	}
	if(len != (uint16_t)(12+pkt->length+4)){
		free(pkt);
		return E_UNCONSISTENT; // or E_LENGTH ??
	}
	if(pkt->length == 0){
		// here, we're sure there is a crc2, no sense if no payload
		free(pkt);
		return E_UNCONSISTENT;
	}
	//TODO : if payload NULL malloc, if not, realloc
	pkt->payload = (char*) malloc(pkt->length);
	if(!pkt->payload){
		free(pkt);
		return E_NOMEM;
	}
	memcpy(pkt->payload, data+12, pkt->length); //payload + crc2
	memcpy(&pkt->crc2, data+12+pkt->length, 4);
	pkt->crc2 = ntohl(pkt->crc2);
	if(crc32(0, (const Bytef*)pkt->payload, pkt->length) != pkt->crc2){ // 8 bytes -> 64 bits
		free(pkt->payload);
		free(pkt);
		return E_CRC;
	}
	return PKT_OK;

}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
	if(!pkt){
		return E_UNCONSISTENT; // sure ?
	}
	int allocsize = 12;
	if(pkt->length != 0){
		allocsize += pkt->length + 4; // payload+crc2
	}
	if((uint16_t)*len < allocsize){
		return E_NOMEM;
	}
	//buf = (char*)malloc(allocsize); // déjà malloc ?
	memcpy(buf, pkt, sizeof(uint8_t)); // type footer window
	memcpy(buf+1, (void*)pkt+1, sizeof(uint8_t)); //seqnum
	uint16_t NBOlength = htons(pkt->length); // pour le memcpy et le crc
	memcpy(buf+2, &NBOlength, sizeof(uint16_t)); // length
	memcpy(buf+4, (void*)pkt+4, sizeof(uint32_t)); // timestamp
	//TODO : here, calculation not needed because it will be done in set crc etc
	/* To calculate crc1, footer has to be set to 0 */
	//pkt->type = 0;
	//pkt->crc1 = crc32(0, pkt, 8); // 0 or 0L as first argument of crc32
	uint8_t typeTrWinTmp;
	memcpy(&typeTrWinTmp, buf, sizeof(uint8_t));
	uint8_t oneWithoutTr = 0xdf; // 11011111
	uint8_t bufTTWT = typeTrWinTmp & oneWithoutTr;
	memcpy(buf, &bufTTWT, sizeof(uint8_t));
	uint32_t sCrc1 = crc32(0, (Bytef *)buf, 8);
	sCrc1 = htonl(sCrc1);
	memcpy(buf+8, &sCrc1, sizeof(uint32_t)); // crc1

	memcpy(buf, &typeTrWinTmp, sizeof(uint8_t));

	/* Now we reset footer as before + length -> ntohs */
	//memcpy(pkt, buf, sizeof(uint_8t)); // TODO : could be avoided if we separate the 2 cases
	//pkt->length = ntohs(pkt->length);

	if(allocsize > 12){
		memcpy(buf+12, pkt->payload, pkt->length); // payload
		//pkt->crc2 = crc32(0, pkt+12, pkt->length);
		uint32_t sCrc2 = crc32(0, (Bytef *)pkt->payload, pkt->length);
		sCrc2 = htonl(sCrc2);
		memcpy(buf+12+pkt->length, &sCrc2, sizeof(uint32_t)); // crc2
	}

	*len = allocsize;
	return PKT_OK;
}

ptypes_t pkt_get_type  (const pkt_t* pkt)
{
	return pkt->type; // ok or do we have to translate ??
}

uint8_t  pkt_get_tr(const pkt_t* pkt)
{
	return pkt->trFlag;
}

uint8_t  pkt_get_window(const pkt_t* pkt)
{
	return pkt->window;
}

uint8_t  pkt_get_seqnum(const pkt_t* pkt)
{
	return pkt->seqNum;
}

uint16_t pkt_get_length(const pkt_t* pkt)
{
	return pkt->length;
}

uint32_t pkt_get_timestamp   (const pkt_t* pkt)
{
	return pkt->timestamp;
}

uint32_t pkt_get_crc1   (const pkt_t* pkt)
{
	return pkt->crc1;
}

uint32_t pkt_get_crc2   (const pkt_t* pkt)
{
	return pkt->crc2; // returns 0 if crc2 isn't present, right ?
}

const char* pkt_get_payload(const pkt_t* pkt)
{
	return pkt->payload; // returns 0 (NULL) if not present right ?
}


pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
	if(type > 3 || type < 0){
		// never happens since we have a ptypes_t ?
		return E_TYPE;
	}
	pkt->type = type; // TODO : working ? yes : see bitfieldsTest.c
	return PKT_OK;
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr)
{
	// and if there is a incompatibilty between tr and payload/crc2 ?
	if(tr!= 0 && tr != 1){
		return E_TR;
	}
	pkt->trFlag = tr;
	return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
	if(window > MAX_WINDOW_SIZE){
		return E_WINDOW;
	}
	pkt->window = window;
	return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
	// if there is an incompatibilty between seqnum and type ?
	pkt->seqNum = seqnum;
	return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
	if(length > MAX_PAYLOAD_SIZE){
		return E_LENGTH;
	}
	pkt->length = length;
	return PKT_OK;
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp)
{
	pkt->timestamp = timestamp;
	return PKT_OK;
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1)
{
	pkt->crc1 = crc1;
	return PKT_OK;
}

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2)
{
	pkt->crc2 = crc2;
	return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t *pkt,
							    const char *data,
								const uint16_t length)
{
	if(length > MAX_PAYLOAD_SIZE){
		return E_LENGTH;
	}
	if(!pkt->payload){
		pkt->payload = (char*)malloc(length);
		if(!pkt->payload){
			return E_NOMEM;
		}
	}
	else{
		if(realloc((void*)pkt->payload,length) == NULL){
			return E_NOMEM;
		}
	}
	memcpy(pkt->payload, data, length);
	pkt->length = length;
	return PKT_OK;
}

void pkt_print(pkt_t* pkt){
	printf("Type : %d\n",pkt->type);
	printf("tr : %d\n",pkt->trFlag);
	printf("window : %d\n",pkt->window);
	printf("seqnum : %d\n",pkt->seqNum);
	printf("length : %d\n",pkt->length);
	printf("timestamp : %d\n",pkt->timestamp);
	printf("crc1 : %u\n",pkt->crc1);
	printf("payload : %.*s\n",pkt->length,pkt->payload);
	printf("crc2 : %u\n",pkt->crc2);
}

/*int main (int argc, char* argv[]){
	pkt_t *pkt = pkt_new();
	pkt_set_type(pkt, PTYPE_DATA);
	pkt_set_tr(pkt,0);
	pkt_set_window(pkt,13);
	pkt_set_seqnum(pkt,1);
	pkt_set_length(pkt,12);
	pkt_set_timestamp(pkt,3);
	pkt->length = htons(pkt->length);*/

	/*
	printf("JUST BEFORE CALCULATING CRC1 :\n");pkt_print(pkt);
	pkt_set_crc1(pkt, crc32(0, (const Bytef*)pkt, 8));
	printf("CRC CALCULATED %u, crc32 : %u\n",pkt->crc1,crc32(0, (const Bytef*)pkt, 8));
	*/
	/*pkt->crc1 = 0;
	pkt->length = ntohs(pkt->length);
	int ret = pkt_set_payload(pkt, "hello world",12);
	printf("============ ret pkt : %d ========\n",ret);
	pkt_set_crc2(pkt, crc32(0, (const Bytef*)pkt->payload, pkt->length));
	printf("pkt1 :\n"); pkt_print(pkt);
	printf("crc32(payload : %.*s) : %u\n",12,pkt->payload,crc32(0,(const Bytef*)pkt->payload,12));

	printf("\n\n\n");

	pkt_t *pkt2 = pkt_new();
	char *buf = malloc(90);
	size_t len = 90;
	printf("====== Result of encode : %d\n",pkt_encode(pkt,buf,&len));
	printf("pkt1 :\n"); pkt_print(pkt);
	//printf("buf : %.*s\n",len,buf);
	printf("crc32(payload : %.*s) : %u\n",12,pkt->payload,crc32(0,(const Bytef*)pkt->payload,12));
	printf("====== Result of decode : %d\n",pkt_decode(buf,len,pkt2));
	printf("pkt1 :\n"); pkt_print(pkt);
	printf("pkt2 :\n"); pkt_print(pkt2);
	return 0;
}*/
