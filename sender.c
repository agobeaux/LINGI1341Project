#include "sender.h"
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

#define ser_PORT 12345


//Resolve the resource name to an usable IPv6 address
const char * real_address(const char *address, struct sockaddr_in6 *rval){
    struct addrinfo hints;
    struct addrinfo *res;
    
    hints.ai_family = AF_INET6;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    
    int err = getaddrinfo(address,NULL,&hints, &res);
    if(err!=0){
        fprintf(stderr, "Problem in getaddrinfo\n");
        return gai_strerror(err);
    }
    
    memcpy(rval, res->ai_addr, res->ai_addrlen);
    
    freeaddrinfo(res);
    return NULL;
}

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




int main(int argc, char *argv[]){
    
    int fd;//file from command line
    char *res_hostname;//hostname from command line
    char *ser_hostname = "::1";
    int dst_port;//port from command line
    int socket_fd;//socket file descriptor
    size_t length = sizeof(char)*20;
    char *info = (char *)malloc(length); // !!!!!!!!not sure of the value
    
    
    
    //check if there is enough arguments to continue
    if (argc<2){
        fprintf(stderr, "There is not enough arguments!\n");
        return EXIT_FAILURE;
    }
    
    //register file if there are an -f option
    int opt;
    int f_option = 0;
    while ((opt = getopt(argc, argv, "f:")) != -1){
        switch(opt){
            case 'f':
                f_option = 1;
                fd = open(optarg, O_RDONLY);
                fprintf(stderr, "Find an f option!\n");
                if(fd == -1){
                    fprintf(stderr, "Can't open the file!\n");
                    exit(EXIT_FAILURE);
                }
                break;
            default:
                fprintf(stderr, "There is an unknown option!\n");
                exit(EXIT_FAILURE);
        }
    }
    //register hostname && port
    if(f_option==0){
        fprintf(stderr, "I'm in no-f option!\n");
        res_hostname = argv[1];
        dst_port = atoi(argv[2]);
        int read = getline(&info, &length, stdin);
    }
    else{
        fprintf(stderr, "I'm in f option!\n");
        res_hostname = argv[4];
        dst_port = atoi(argv[5]);
    }
    
    
    //Resolve the resource name to an usable IPv6 address
    struct sockaddr_in6 addr;
    const char *check_message = real_address(res_hostname, &addr);
    if(check_message){
        fprintf(stderr, "Problem in getaddrinfo %s!\n",check_message);
    }
    
    //Creates a socket and initializes it
    socket_fd = create_socket(NULL, -1, &addr, dst_port);//réecrire
    if(socket_fd == -1){
        fprintf(stderr, "Failed to create the socket!\n");
        exit(EXIT_FAILURE);
    }
    
    
    /* DO THINGS */
    
    
    char *buf = "Lily";
    write(socket_fd,(void *) buf, 5);
    
    close(socket_fd);
    close(fd);
    return 0;
}
