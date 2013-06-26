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
int pid;
void handler() {
	if (pid)
        kill(-pid, SIGINT);
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
		    if (fork()) {
		        close(new_sd);
		    } else {
                close(sd);
                int master, slave;
                char buffer[4096];
                if (openpty(&master, &slave, buffer, NULL, NULL) != 0) {
                    close(new_sd);
                    return 7;
                }
                if (fork()) {
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

                } else {
                    close(master);
	                dup2(slave, 0);
	                dup2(slave, 1);
	                dup2(slave, 2);
	                setsid();
	                close(new_sd);
	                int ff = open(buffer, O_RDWR);
	                if(ff)
	                    close(ff);
	                execl("/bin/bash", "/bin/bash", NULL);
	                return 0;
                }
		        return 0;
		    }
            return 0;
		}
	    
	    close(sd);
	    return 0;

    }
}
