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

#define UNUSED(x) (void) x;
void write_to_fd(int fd, char * buffer, int len) {
    int num_ok = 0;
    while (num_ok < len) {
        num_ok += write(fd, buffer + num_ok, len - num_ok);
    }
}

void read_until_char(int fd, char * buffer, int * len, char * rem, int * rem_size, char c) {
	int was_zero = 0, i, j;
	for (i = 0; i < (*len); i++) {
		if (buffer[i] == c) {
			(*len) = i - 1;
            for (j = i + 1; j < (*len); j++) {
                rem[j - (i + 1)] = buffer[j];
            }
            (*rem_size) = (*len) - (i);
            was_zero = 1;
            break;                
		}
	}
	if (was_zero) {
		return;
	}

	while (1) {
		int sz = read(fd, buffer + (*len), MAX_SIZE - (*len));
		(*len) += sz;
		if (sz <= 0) {
			return;
		}
		int was_zero = 0;
		for (i = 0; i < (*len); i++) {
			if (buffer[i] == c) {
				(*len) = i;
                for (j = i + 1; j < (*len); j++) {
                    rem[j - (i + 1)] = buffer[j];
                }
                (*rem_size) = (*len) - (i);
                was_zero = 1;
                break;                
			}
		}
		if (was_zero) {
			break;
		}
	}
}

int main(int argc, char ** argv) {
	UNUSED(argc);
    if ((pid = fork())) {
        signal(SIGINT, &handler); 
        wait(NULL);
        return 0;   
    } else {
        int status, sd;
        struct addrinfo hints, *res;
        setsid();

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
        write_to_fd(sd, argv[1], strlen(argv[1]) + 1);
        char * message = (char *) malloc(MAX_SIZE);
        char * correct_size = (char *) malloc(MAX_SIZE);
        char * data = (char *) malloc(MAX_SIZE);

        int message_len = 0, correct_size_len = 0, data_len = 0;
        read_until_char(sd, message, &message_len, correct_size, &correct_size_len, '\0');
        if (message[0] == ERROR_CHAR) {
        	write_to_fd(1, message, message_len);
        	close(sd);
        	return 0;
        }
        read_until_char(sd, correct_size, &correct_size_len, data, &data_len, '\0');
        read_until_char(sd, data, &data_len, data, &data_len, '\0');

        int num = atoi(correct_size);
        if (num == data_len) {
       	    write_to_fd(1, data, data_len);
        } else {
            write_to_fd(1, "IO error", 8);
        }
        

        close(sd);
        return 0;

    }
}
