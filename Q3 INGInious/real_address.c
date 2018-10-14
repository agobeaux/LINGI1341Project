#include "real_address.h"
#include <stdio.h>

const char * real_address(const char *address, struct sockaddr_in6 *rval){
    // UDP : Datagram -> datagram sockets ?
    struct addrinfo hints;
    struct addrinfo *result;
    int s;

    memset(&hints, 0, sizeof(struct addrinfo)); // to be sure that memory is set to 0
    hints.ai_family = AF_INET6;    /* Allow IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket, since it's UDP no ? */
    /* Following lines : useless because of memset 0 */
    //hints.ai_flags = 0;    /* ? */
    //hints.ai_protocol = 0;          /* Any protocol, is it ok ? */

    // int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
    s = getaddrinfo(address, NULL, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return(gai_strerror(s));
        //exit(EXIT_FAILURE);
    }
    //rval = (struct sockaddr_in6 *)result->ai_addr;
    memcpy(rval, result->ai_addr, result->ai_addrlen);
    freeaddrinfo(result);
    fprintf(stderr, "rval a été set, flowinfo: %d\n",rval->sin6_flowinfo);
    return NULL;
}
