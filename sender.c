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
//Creates a socket and initialize it
int create_socket(struct sockaddr_in6 *dest_addr, int dst_port){
    
    int domain = AF_INET6;
    int type = SOCK_DGRAM;
    int protocol = IPPROTO_UDP;
    
    int sockfd = socket(domain,type,protocol);
    if(sockfd<0){
        fprintf(stderr, "problem with socket\n");
        return -1;
    }
    
    if(dest_addr!=NULL){
        
        if(dst_port>0){dest_addr->sin6_port=htons(dst_port);}
        
        int c = connect(sockfd, (const struct sockaddr *)dest_addr,sizeof(struct sockaddr_in6));
        if(c<0){
            fprintf(stderr, "problem with connect\n");
            return -1;
        }
    }
    return sockfd;
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
                fd = open(optarg, O_RDWR);
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
        res_hostname = argv[0];
        dst_port = atoi(argv[1]);
        if(argc>2 && (strcmp(argv[2], "<") == 0)){
            fd = open(argv[3], O_RDWR);
            if(fd == -1){
                fprintf(stderr, "Can't open the file!\n");
                exit(EXIT_FAILURE);
            }
        }
        int read = getline(&info, &length, stdin);
        if (read == -1)
        {
            fprintf(stderr, "Error with getline in read_console_input.\n");
            exit(EXIT_FAILURE);
        }
        write(fd, info, sizeof(char)*20);
        
    }
    else{
        fprintf(stderr, "I'm in f option!\n");
        res_hostname = argv[3];
        dst_port = atoi(argv[4]);
    }
    
    
    //Resolve the resource name to an usable IPv6 address
    struct sockaddr_in6 *rval = (struct sockaddr_in6 *)malloc(sizeof(struct sockaddr_in6));
    const char *check_message = real_address(res_hostname, rval);
    if(real_address(res_hostname, rval)!=NULL){
        fprintf(stderr, "Problem in getaddrinfo %s!\n",check_message);
    }
    
    //Creates a socket and initialize it
    socket_fd = create_socket(rval, dst_port);
    if(socket_fd == -1)exit(EXIT_FAILURE);
    
    const void *buf = "Lily";
    write(socket_fd, buf, sizeof(*buf));
    
    close(socket_fd);
    close(fd);
    free(rval);
}
