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

#define UNUSED(x) (void) x;
void write_to_fd(int fd, char * buffer, int len) {
    int num_ok = 0;
    while (num_ok < len) {
        num_ok += write(fd, buffer + num_ok, len - num_ok);
    }
}

int main(int argc, char ** argv) {
    UNUSED(argc);
    int status, sd;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    
    hints.ai_socktype = SOCK_STREAM;    
    hints.ai_flags = AI_PASSIVE;   

    if ((status = getaddrinfo(NULL, argv[2], &hints, &res)) != 0) {
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
    pollfd
    while (1) {

    }
    write_to_fd(1, "after connection\n", 17);
    write_to_fd(sd, argv[1], strlen(argv[1]) + 1);
    char * message = (char *) malloc(MAX_SIZE);
    char * correct_size = (char *) malloc(MAX_SIZE);
    char * data = (char *) malloc(MAX_SIZE);

    int message_len = 0, correct_size_len = 0, data_len = 0;
    read_until_char(sd, message, &message_len, correct_size, &correct_size_len, '\0');
    
    write_to_fd(1, "after read_until1\n", 18);
    if (message[0] == ERROR_CHAR) {
        write_to_fd(1, message, message_len);
        close(sd);
        return 0;
    }
    read_until_char(sd, correct_size, &correct_size_len, data, &data_len, '\0');
    message_len = 0;
    read_until_char(sd, data, &data_len, message, &message_len, '\0');
    write_to_fd(1, "after read_until2\n", 18);
    printf("%s\n", correct_size);
    int num = atoi(correct_size);
    if (num == data_len) {
        write_to_fd(1, data, data_len);
    } else {
        write_to_fd(1, "IO error", 8);
    }
     

    close(sd);
    return 0;
}
