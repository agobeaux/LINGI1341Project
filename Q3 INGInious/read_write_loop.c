#include "read_write_loop.h"
#include <stdio.h>
#include <unistd.h> // STDIN_FILENO
#include <poll.h>
#include <errno.h> /* for errno */
#include <string.h> /* for strerror */

/* Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */
void read_write_loop(const int sfd){
	struct pollfd pfds[3];
	pfds[0].fd = STDIN_FILENO;
	pfds[0].events = POLLIN;
	pfds[0].revents = 0;
	pfds[1].fd = sfd;
	pfds[1].events = POLLIN|POLLOUT;
	pfds[1].revents = 0;
	pfds[2].fd = STDOUT_FILENO;
	pfds[2].events = POLLOUT;
	pfds[2].revents = 0;
	
	while(1){
		int pRet = poll(pfds, 3, 5);
		if(pRet == -1){
			fprintf(stderr, "Error poll call: %s\n", strerror(errno));
		}
		char buf[1024];
		int readSth = 1; int wroteSth = 1;
		if(pfds[0].revents & POLLIN && pfds[1].revents & POLLOUT){
			int rd = read(STDIN_FILENO, (void*)buf, 1024);
			if(rd == -1){
				fprintf(stderr, "Stdin->socket : Read error : %s\n", strerror(errno));
			}
			else if(rd == 0){
				readSth = 0;
				// EOF
				//TODO
			}
			else{
				int wr = write(sfd, (void*)buf, rd);
				if(wr == -1){
					fprintf(stderr, "Stdin->socket : Write error : %s\n", strerror(errno));
				}
				else if(wr < rd){
					fprintf(stderr, "Stdin->socket : Nb of bytes written < nb of bytes read\n");
				}
			}
		}
		if(pfds[1].revents & POLLIN && pfds[2].revents & POLLOUT){
			int rd = read(sfd, (void*)buf, 1024);
			if(rd == -1){
				fprintf(stderr, "Socket->Stdout : Read error : %s\n", strerror(errno));
			}
			else if(rd == 0){
				wroteSth = 0;
				// EOF
				//TODO
			}
			else{
				int wr = write(STDOUT_FILENO, (void*)buf, rd);
				if(wr == -1){
					fprintf(stderr, "Socket->Stdout : Write error : %s\n", strerror(errno));
				}
				else if(wr < rd){
					fprintf(stderr, "Socket->Stdout : Nb of bytes written < nb of bytes read\n");
				}
			}
		}
		if(!readSth && !wroteSth){
			break;
		}
	}
	return;
}
