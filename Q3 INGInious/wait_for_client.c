#include "wait_for_client.h"
#include <stdio.h>
#include <errno.h> /* for errno */
#include <string.h> /* for strerror */
#include <netinet/in.h> /* * sockaddr_in6 */
#include <sys/types.h> /* sockaddr_in6 + connect */
#include <sys/socket.h> /* connect */

/* Block the caller until a message is received on sfd,
 * and connect the socket to the source address of the received message.
 * @sfd: a file descriptor to a bound socket but not yet connected
 * @return: 0 in case of success, -1 otherwise
 * @POST: This call is idempotent, it does not 'consume' the data of the message,
 * and could be repeated several times blocking only at the first call.
 */
int wait_for_client(int sfd){
	// flag -> 0 ?
	struct sockaddr_in6 source_addr;
	socklen_t fromlen;
	fromlen = sizeof(struct sockaddr_in6);
	// MSG_PEEK : so that it doesn't remove data
	int ret = recvfrom(sfd, NULL, 0, MSG_PEEK, (struct sockaddr *) &source_addr, &fromlen);
	if(ret == -1){
		fprintf(stderr, "Error for recvfrom call : %s\n", strerror(errno)); // perror(); ?
		return -1;
	}
	
	if(connect(sfd, (struct sockaddr *) &source_addr, sizeof(struct sockaddr_in6)) == -1){
		fprintf(stderr, "Error for connect call : %s\n", strerror(errno)); // perror(); ?
		return -1;
	}
	return 0;
}
