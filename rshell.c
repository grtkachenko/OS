#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
int main(int argc, char ** argv) {
    struct addrinfo ** res;
    
        
    int sd = socket(PF_INET, SOCK_STREAM, 0);
}
