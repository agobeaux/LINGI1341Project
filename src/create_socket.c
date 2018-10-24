#include "create_socket.h"
#include <stdio.h>
#include <errno.h> /* for errno */
#include <string.h> /* for strerror */
#include <unistd.h>

/* Creates a socket and initialize it
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listening
 * @dest_addr: if !NULL, the destination address to which the socket should send data
 * @dst_port: if >0, the destination port to which the socket should be connected
 * @return: a file descriptor number representing the socket,
 *         or -1 in case of error (explanation will be printed on stderr)
 */
int create_socket(struct sockaddr_in6 *source_addr,
                 int src_port,
                 struct sockaddr_in6 *dest_addr,
                 int dst_port){
					 if(source_addr && src_port > 0 && dest_addr && dst_port > 0){
						 fprintf(stderr, "Both source_addr and dest_addr are available, is it normal ?\n");
						 return -1;
					 }
					 struct protoent *proto = getprotobyname("udp");
					 // getprotobyname returns a pointer to a statically allocated protoend structure
					 // FREE PROTO ???
					 if(!proto){
						 fprintf(stderr, "Could not find proto with name \"udp\"\n");
						 return -1;
					 }
					 int sockfd = socket(AF_INET6, SOCK_DGRAM, proto->p_proto);
					 if(sockfd == -1){
						 fprintf(stderr, "Error for socket call : %s\n", strerror(errno)); // perror(); ?
						 return -1;
					 }
					 if(source_addr && src_port > 0){
						 // sin6_port ?... shoudl be in network byte order apparently htons ?
						 // positive int -> transforme into uint32_t. max int 2^31-1
						 // in_port_t similar to uint16_t... not uint32_t...
						 
						 // (unsigned short int) host byte order -> network byte order
						 source_addr->sin6_port = htons(src_port);
                         source_addr->sin6_family = AF_INET6;
						 // source_addr->in_port_t = src_port;
						 if(bind(sockfd, (struct sockaddr *) source_addr, sizeof(struct sockaddr_in6)) == -1){
							 fprintf(stderr, "Error for bind call : %s\n", strerror(errno)); // perror(); ?
						 close(sockfd); //TODO : right ?
							 return -1;
						 }
						 return sockfd;
					 }
					 
					 if(dest_addr && dst_port > 0){
						 dest_addr->sin6_port = htons(dst_port);
						 
						 if(connect(sockfd, (struct sockaddr *) dest_addr, sizeof(struct sockaddr_in6)) == -1){
							 fprintf(stderr, "Error for connect call : %s\n", strerror(errno)); // perror(); ?
						 close(sockfd);
							 return -1;
						 }
						 return sockfd;
					 }
					 close(sockfd); // check at first ?
					 fprintf(stderr, "Couldn't find at a pair (addr, port)\n");
					 return -1;
				 }
