//
//  receiver.h
//
//
//  Created by Лилия Семерикова on 14/10/2018.
//

#ifndef receiver_h
#define receiver_h

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
#include "packet_interface.h"

#define MAX_PACKET_SIZE 528
#define ACK_SIZE 12
#define MAX_BUFFER_SIZE 31

/**
 * Resolve the resource name to an usable IPv6 address
 *
 * @address: The name to resolve
 * @rval: Where the resulting IPv6 address descriptor should be stored
 *
 * @return : return: NULL if it succeeded, or a pointer towards a string describing the error if any.
 */
const char * real_address(const char *address, struct sockaddr_in6 *rval);

/**
 * Creates a socket and initialize it
 *
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listenin
 * @dest_addr: if !NULL, the destination address to which the socket should send data
 * @dst_port: if >0, the destination port to which the socket should be connected
 *
 * @return : a file descriptor number representing the socket, or -1 in case of error (explanation will be printed on stderr)
 */
int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port);

/**
 * Loop for writing packets and reading the acks/nacks
 *
 * @sfd socket's file descriptor
 * @fd file descriptor containing the informations to send
 */
void read_write_loop(const int sfd, const int fd);

#endif /* receiver_h */
