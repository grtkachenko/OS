#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <util.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
const int MAX_SIZE = 4096;
const char ERROR_CHAR = 'e';
int pid;
void handler() {
    if (pid)
        kill(pid, SIGINT);
}

int main(int argc, char ** argv) {
    (void) argc;
    int status, sd;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    
    hints.ai_socktype = SOCK_STREAM;    
    hints.ai_flags = AI_PASSIVE;   

    if ((status = getaddrinfo(NULL, argv[1], &hints, &res)) != 0) {
        perror("error getaddrfinfo");
        return 1;
    } 
    if ((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        perror("error socket");
        return 2;
    } 

    status = 1;
    if (connect(sd, res->ai_addr, res->ai_addrlen) == -1) {
        close(sd);
        perror("connect fail");
        return 3;
    }
    pollfd clients[MAX_CLIENTS];
    clients[0].fd = sd;
    clients[0].events = POLLIN | POLLOUT;

    while (1) {
        poll(clients, num_clients, -1);
        if (client[i].revents & POLLIN) {
            

        }
        if (client[i].revents & POLLOUT) {

        }


    }
     

    close(sd);
    return 0;
}
