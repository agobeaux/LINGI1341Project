#include "real_address.h"
#include <stdio.h>

//Resolve the resource name to an usable IPv6 address
const char * real_address(const char *address, struct sockaddr_in6 *rval){
    struct addrinfo hints;
    struct addrinfo *res;

    hints.ai_family = AF_INET6;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // check if needed

    int err = getaddrinfo(address, NULL, &hints, &res);
    if(err!=0){
        return gai_strerror(err);
    }

    memcpy(rval, res->ai_addr, res->ai_addrlen);

    freeaddrinfo(res);
    return NULL;
}
