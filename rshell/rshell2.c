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
const int MAX_SIZE = 4096;

int ppd;

void handler(int a) {
    if(ppd) {
        kill(-ppd, a);
    }
}

int read_to(int fd, char * buf, int * len) {
    int size = read(fd, buf + (*len), MAX_SIZE); 
    if (size == -1) {
        return -1;
    }
    (*len) += size;
    return 0;
}

int write_to(int fd, char * buf, int * len) {
    int size = write(fd, buf, (*len)); 
    if (size == -1) {
        return -1;
    }
    memmove(buf, buf + size, (*len) - size);
    (*len) -= size;
    return 0;
}

short get_event(int len) {
    short event = POLLERR | POLLHUP;
    if (len != 0) {
        event |= POLLOUT;
    }
    if (len != MAX_SIZE) {
        event |= POLLIN;
    }
    return event;
}


int main() {
    close(0);
    close(1);
    close(2);
    ppd = fork();
    if(ppd) {
        signal(SIGINT, &handler);
        wait(NULL);
        return 0;
    }
    setsid();
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int status;
    status = getaddrinfo(0, "8824", &hints, &res);
    if(status != 0) {
        perror("getaddrinfo failed");
        return 1;
    }
    int sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sd == -1) {
        perror("ERROR opening socket");
        return 1;
    }
    status = bind(sd, res->ai_addr, res->ai_addrlen);
    if(status == -1) {
        close(sd);
        perror("bind failed");
        return 1;
    }
    status = 1;
    status = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &status, sizeof(int));
    if(status == -1) {
        close(sd);
        perror("setsockport failed");
        return 1;
    }

    status = listen(sd, 5);
    if(status == -1) {
        close(sd);
        perror("listen failed");
        return 1;
    }

    struct sockaddr_storage remt_addr;
    socklen_t addr_size;
    addr_size = sizeof(remt_addr);

    while(1) {
        int new_sd = accept(sd, (struct sockaddr*) &remt_addr, &addr_size);
        if(new_sd == -1) {
            close(sd);
            perror("accept failed");
            return 1;
        }

        if(fork()) {
            close(new_sd);
        } else {
            close(sd);
            int master, slave;
            char buf[4096];
            if(openpty(&master, &slave, buf, NULL, NULL) != 0) {
                close(new_sd);
                perror("openpty failed");
                return 1;
            }
            if(!fork()) {
                close(master);
                dup2(slave, 0);
                dup2(slave, 1);
                dup2(slave, 2);
                setsid();
                close(new_sd);
                int ff = open(buf, O_RDWR);
                if(ff)
                    close(ff);
                execl("/bin/bash", "/bin/bash", NULL);
                return 0;
            } else {
                close(slave);
                fcntl(master, F_SETFL, fcntl(master, F_GETFL) | O_NONBLOCK);
                fcntl(new_sd, F_SETFL, fcntl(new_sd, F_GETFL) | O_NONBLOCK); 
                struct pollfd fds[] = {
                    {
                        master, POLLIN | POLLOUT | POLLERR | POLLHUP, 0
                    }, 
                    {
                        new_sd, POLLIN | POLLOUT | POLLERR | POLLHUP, 0
                    }
                };
                char bufmaster[MAX_SIZE], bufnew_sd[MAX_SIZE];
                int bufmaster_len = 0, bufnew_sd_len = 0;
                while (1) {
                    fds[0].events = get_event(bufmaster_len);
                    fds[1].events = get_event(bufnew_sd_len);
                    int num = poll(fds, 2, -1);
                    if (num == -1) {
                        if (errno == EINTR) {
                            continue;
                        } else {
                            close(master);
                            close(new_sd);
                            return 8;
                        }
                    }
                    if ((fds[0].revents & (POLLERR | POLLHUP)) == 0 && (fds[0].revents & POLLIN) != 0) {
                        if (read_to(master, bufnew_sd, &bufnew_sd_len) == -1) {
                            close(master);
                            close(new_sd);
                            return 9;
                        }
                        
                    }
                    if ((fds[0].revents & (POLLERR | POLLHUP)) == 0 && (fds[0].revents & POLLOUT) != 0) {
                        if (write_to(master, bufmaster, &bufmaster_len) == -1) {
                            close(master);
                            close(new_sd);
                            return 10;
                        }
                    }
                    if ((fds[1].revents & (POLLERR | POLLHUP)) == 0 && (fds[1].revents & POLLIN) != 0) {
                        if (read_to(new_sd, bufmaster, &bufmaster_len) == -1) {
                            close(master);
                            close(new_sd);
                            return 11;
                        }
                    }
                    if ((fds[1].revents & (POLLERR | POLLHUP)) == 0 && (fds[1].revents & POLLOUT) != 0) {
                        if (write_to(new_sd, bufnew_sd, &bufnew_sd_len) == -1) {
                            close(master);
                            close(new_sd);
                            return 12;
                        }
                    }
                }
           }

            close(new_sd);
            return 0;
        }
    }

    close(sd);
    return 0;
}