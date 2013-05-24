#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
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
		        dup2(new_sd, 0);
		        dup2(new_sd, 1);
		        dup2(new_sd, 2);
		        close(new_sd);
		        printf("Hello\n");
		        return 0;
		    }
		}
	    
	    close(sd);
	    return 0;

    }
}
