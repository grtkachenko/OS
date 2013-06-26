#include <sys/socket.h>
#include <string>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <signal.h>
#include <util.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <map>
#include <set>
using namespace std;
const int MAX_SIZE = 4096, MAX_CLIENTS = 100;
int pid;
void handler(int val) {
    (void)val;
    if (pid)
        kill(pid, SIGINT);
}
struct my_buffer {
    char * buffer;
    int len;
    my_buffer() {
        len = 0;
        buffer = (char *) malloc(MAX_SIZE);
    }
    void add(string st) {
        for (int i = len; i < len + (int) st.length(); i++) {
            buffer[i] = st[i - len];
        }
        len += st.length();
        buffer[len++] = '\n';
    }
    ~my_buffer() {
        free(buffer);
    }
};

string parse_it(char * buffer, int len) {
    bool is_ok = false;
    string ev;
    for (int j = 0; j < len; j++) {
        if (buffer[j] == ' ') {
            is_ok = true;
            continue;
        } 
        if (buffer[j] == '\n') {
            is_ok = false;
        }
        if (is_ok) {
            ev += buffer[j];
        }
    }
    return ev;
}
int main() {
    if ((pid = fork())) {
        signal(SIGINT, &handler); 
        wait(NULL);
        return 0;   
    } else {
        int status, sd;
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
        my_buffer out_to_client[MAX_CLIENTS];
        pollfd clients[MAX_CLIENTS];
        int num_clients = 1;
        clients[0].fd = sd;
        clients[0].events = POLLIN;
        map<int, set<string> > map_clients;

        while (1) {
            poll(clients, num_clients, -1); 
            for (int i = 1; i < num_clients; i++) {
                if (clients[i].revents & POLLIN) {
                    char buffer[50];
                    int cnt = read(clients[i].fd, buffer, 50);
                    if (cnt == 0) {
                        if (clients[i].events & POLLOUT) {
                            clients[i].events = POLLOUT;
                        } else {
                            clients[i].events = 0;
                        }
                    } else {
                        if (buffer[0] == 's') {
                            map_clients[i].insert(parse_it(buffer, 50));
                            continue;
                        } 
                        if (buffer[0] == 'u') {
                            map_clients[i].erase(parse_it(buffer, 50));
                            continue;
                        }

                        if (buffer[0] == 'l') {
                            for (string st : map_clients[i]) {
                                if (out_to_client[i].len == 0) {
                                    clients[i].events |= POLLOUT;
                                }
                                out_to_client[i].add(st);
                            }
                            continue;
                        }
                        if (buffer[0] == 'e') {
                            string st = parse_it(buffer, 50);
                            for (int j = 1; j < num_clients; j++) {
                                if (map_clients[j].count(st) != 0) {
                                    if (out_to_client[i].len == 0) {
                                        clients[i].events |= POLLOUT;
                                    }
                                    out_to_client[j].add(st);
                                }
                            }
                        }
                    }

                }
                if (clients[i].revents & POLLOUT) {
                    int cnt = write(clients[i].fd, out_to_client[i].buffer, out_to_client[i].len);
                    memmove(out_to_client[i].buffer, out_to_client[i].buffer + cnt, out_to_client[i].len - cnt); 
                    out_to_client[i].len -= cnt;
                    if (out_to_client[i].len == 0) {
                        if (clients[i].events | POLLIN) {
                            clients[i].events = POLLIN;
                        } else {
                            clients[i].events = 0;
                        }
                         
                    }

                }

            }
            if (clients[0].revents & POLLIN) {
                socklen_t  addr_size = sizeof(sock_stor);
                clients[num_clients].fd = accept(sd, (struct sockaddr *) &sock_stor, &addr_size);
                clients[num_clients].events = POLLIN;
                num_clients++;
            }
        }
        
        close(sd);
        return 0;

    }
}
