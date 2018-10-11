#include "record.h"

/**
 * Initialise un enregistrement
 * @pre: r!= NULL
 * @post: record_get_type(r) == 0 && record_get_length(r) == 0
 *           && record_has_footer(r) == 0
 * @return: 1 en cas d'erreur, 0 sinon
 */
 int record_init(struct record *r){
     r->type = 0;
     r->f = 0;
     r->length = 0;
     r->uuid = 0;
     r->payload = NULL;
     return 0;
 }

/**
 * Libère les ressources consommées par un enregistrement
 * @pre: r!= NULL
 */
 void record_free(struct record *r){
     if(!r->payload)
         free(r->payload);
 }

/**
  * Renvoie le type d'un enregistrement
  * @pre: r != NULL
  */
 int record_get_type(const struct record *r){
     return r->type;
 }

 /**
  * Définit le type d'un enregistrement
  * @pre: r != NULL
  * @post: record_get_type(r) == (type & 0x7FFF)
  */
 void record_set_type(struct record *r, int type){
     // type & 0x7FFF ??? Ok ! 1 partout, 15 bits de long
     r->type = type;
 }

 /**
  * Renvoie la taille du payload de l'enregistrement (dans l'endianness native de la machine!)
  * @pre: r!= NULL
  */
 int record_get_length(const struct record *r){
     // endianness native de la machine ? -> fonction cheloue à utiliser ?
     return r->length;
 }


 /**
  * Définit le payload de l'enregistrement, en copiant n octets du buffer. Si le buffer est NULL (ou de taille 0), supprime le payload
  * @pre: r != NULL && buf != NULL && n > 0
  * @post: record_get_length(r) == (n & 0xFFFF)
              && record_get_payload(<buf2>, n) == (n & 0xFFFF)
  *        && memcmp(buf, <buf2>, (n & 0xFFFF)) == 0
  * @return: -1 en cas d'erreur, 0 sinon
  */
 int record_set_payload(struct record *r, const char * buf, int n){
     //TODO : remplace le payload ou ajoute ? genre on doit realloc ou alloc ?
     //TODO : DO (malloc todo too) + verif conditions
     //TODO LENGTH EST UN TYPE SPECIAL !!! FONCTION A UTILISER POUR L ENDIANNESS
     r->payload = (char*)malloc(sizeof(char)*n);
     if(!r->payload)
         return -1;
     memcpy(r->payload, buf, n);
     r->length = n;
     return 0;
 }

 /**
  * Copie jusqu'à n octets du payload dans un buffer
  * pré-alloué de taille n
  * @pre: r != NULL && buf != NULL && n > 0
  * @return: n', le nombre d'octets copiés dans le buffer
  * @post: n' <= n && n' <= record_get_length(r)
  */
 int record_get_payload(const struct record *r, char *buf, int n){
     // pq n' <= record_get_length(r) ??? pq pas juste = ?
     int bufsize = r->length < n ? r->length : n;
     memcpy(buf, r->payload, bufsize);
     return bufsize;
 }

 /**
  * Teste si l'enregistrement possède un footer
  * @pre: r != NULL
  * @return: 1 si l'enregistrement a un footer, 0 sinon
  */
 int record_has_footer(const struct record *r){
     return r->f == 1;
 }

 /**
  * Supprime le footer d'un enregistrement
  * @pre: r != NULL
  * @post: record_has_footer(r) == 0
  */
 void record_delete_footer(struct record *r){
     r->f = 0;
     r->uuid = 0;
 }

 /**
  * Définit l'uuid d'un enregistrement
  * @pre: r != NULL
  * @post: record_has_footer(r) &&
           record_get_uuid(r, &<uuid2>) => uuid2 == uuid
  */
 void record_set_uuid(struct record *r, unsigned int uuid){
     r->f = 1;
     r->uuid = uuid;
 }

 /**
  * Extrait l'uuid d'un enregistrement
  * @pre: r != NULL
  * @post: (record_has_footer(r) && uuid != 0) ||
  *        (!record_has_footer(r) && uuid == 0)
  */
 unsigned int record_get_uuid(const struct record *r){
     return r->uuid;
 }

/**
 * Ecrit un enregistrement dans un fichier
 * @pre: r != NULL && f != NULL
 * @return: n', le nombre d'octets écrits dans le fichier.
           -1 en cas d'erreur
 */
 int record_write(const struct record *r, FILE *f){
	 int bytesWritten = 0;
     /*
     // WARNING !!! : endianness of the machine.
     typeF = typeF << 1;
     typeF = typeF | r->f;
     */
     // WARNING : fwrite returns the numbers of members written ! not the nb of bytes
     bytesWritten += fwrite(r, sizeof(uint16_t), 1, f)*sizeof(uint16_t);

     uint16_t lengthNet = htons(r->length);
     bytesWritten += fwrite((void*)&lengthNet, sizeof(lengthNet), 1, f)*sizeof(lengthNet);

     bytesWritten += fwrite((void*)r->payload, r->length*sizeof(char), 1 , f)*(r->length*sizeof(char));

     if(r->f){
         bytesWritten += fwrite((void*)&(r->uuid), sizeof(r->uuid), 1, f)*sizeof(r->uuid);
     }
     
     return bytesWritten;
 }

/**
 * Lit un enregistrement depuis un fichier
 * @pre: r != NULL && f != NULL
 * @return: n', le nombre d'octets luts dans le fichier.
           -1 en cas d'erreur
 */
 int record_read(struct record *r, FILE *f){
     int bytesRead = 0;
     bytesRead += fread(r, sizeof(uint16_t), 1, f)*sizeof(uint16_t);
     // WARNING : fwrite returns the numbers of members written ! not the nb of bytes
     
     uint16_t lengthNet;
     bytesRead += fread((void*)&lengthNet, sizeof(lengthNet), 1, f)*sizeof(lengthNet);
     r->length = ntohs(lengthNet);
     
     
     r->payload = (char*)malloc(sizeof(char)*r->length);
     bytesRead += fread((void*)r->payload, r->length*sizeof(char), 1 , f)*(r->length*sizeof(char));

     if(r->f){
         bytesRead += fread((void*)&(r->uuid), sizeof(r->uuid), 1, f)*sizeof(r->uuid);
     }
     
     return bytesRead;
 }
