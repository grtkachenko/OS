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
int pid;
void handler() {
    if (pid)
        kill(pid, SIGINT);
}

void write_to_fd(int fd, char * buffer, int len) {
    int num_ok = 0;
    while (num_ok < len) {
        num_ok += write(fd, buffer + num_ok, len - num_ok);
    }
}

void print_error_to_fd(int fd) {
    char * msg = strerror(errno);
    write_to_fd(fd, "err: ", 5);
    write_to_fd(fd, msg, strlen(msg));
}

void print_ok_to_fd(int fd) {
    write_to_fd(fd, "ok\0", 3);
}

void print_int_to_fd(int fd, int val) {
    char val_str[10];
    sprintf(val_str, "%d", val);
    write_to_fd(fd, val_str, strlen(val_str));
    write_to_fd(fd, "\0", 1);
}

int main() {
    if ((pid = fork())) {
        signal(SIGINT, &handler); 
        wait(NULL);
        return 0;   
    } else {
        int status, sd, new_sd;
        struct addrinfo hints, *res;
        struct sockaddr_storage sock_stor;
        setsid();

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;    
        hints.ai_socktype = SOCK_STREAM;    
        hints.ai_flags = AI_PASSIVE;   

        if ((status = getaddrinfo(NULL, "8822", &hints, &res)) != 0) {
            perror("error getaddrfinfo");
            return 1;
        } 
        if ((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
            perror("error socket");
            return 2;
        } 

        status = 1;
        if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &status, sizeof(int)) == -1) {
            close(sd);
            perror("setsockopt fail");
            return 3;
        }

        if (bind(sd, res->ai_addr, res->ai_addrlen) == -1) {
            close(sd);
            perror("bind fail");
            return 4;
        }
        
        if (listen(sd, 5) == -1) {
            close(sd);
            perror("listen fail");
            return 5;
        }
        while (1) {
            socklen_t addr_size = sizeof(sock_stor);
            if ((new_sd = accept(sd, (struct sockaddr *) &sock_stor, &addr_size)) == -1) {
                close(sd);
                perror("accept fail");
                return 6;
            }
            
            if (!fork()) {
                close(sd);
                char * file_name = (char *) malloc(MAX_SIZE);
                int len = 0;
                while (1) {
                    int i, was_zero = 0, sz = read(new_sd, file_name + len, MAX_SIZE  - len);
                    len += sz;
                    for (i = 0; i < len; i++) {
                        if (file_name[i] == '\0') {
                            was_zero = 1;
                            break;
                        }
                    }
                    if (sz <= 0 || was_zero) {
                        break;
                    }
                }
                int fd = open(file_name, O_RDONLY);
                if (fd == -1) {
                    print_error_to_fd(new_sd);
                    free(file_name);
                    close(new_sd);
                    return 0;
                } else {
                    print_ok_to_fd(new_sd);
                    struct stat buf;
                    fstat(fd, &buf);
                    print_int_to_fd(new_sd, buf.st_size);
                    char * buffer = (char *) malloc(MAX_SIZE);
                    while (1) {
                        int sz = read(fd, buffer, MAX_SIZE);
                        if (sz <= 0) {
                            break;
                        }
                        write_to_fd(new_sd, buffer, sz);
                    }
                    write_to_fd(new_sd, "\0", 1);
                    free(buffer);
                }
                free(file_name);
                close(new_sd);
                return 0;
            }
            close(new_sd);
        }
        
        close(sd);
        return 0;

    }
}
