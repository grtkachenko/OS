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
#include <vector>
#include <memory>
#include <iostream>
using namespace std;
const int MAX_SIZE = 4096, MAX_CLIENTS = 100;
const string COLLISION = "\0";

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
    }
    ~my_buffer() {
        free(buffer);
    }
};

// -1 = nothing, 0 = change, 1 = collision, 2 = print
int parse(char * buffer, int cnt) {
    for (int i = 0; i < cnt; i++) {
        if (buffer[i] == '\n') {
            if (buffer[1] == 'r') {
                return 2;
            }
            if (buffer[1] == 'o') {
                return 1;
            }
            if (buffer[1] == 'h') {
                return 0;
            }
            return -1;
        }
    }
    return -1;
}
string get_next(char * buffer, int size, char delim, int number) {
    string ans;
    int cur_num = 0;
    for (int i = 0; i < size; i++) {
        if (buffer[i] == delim) {
            ans += '\0';
            if (cur_num == number) {
                return ans;
            } else {
                ans = "";
            }
            cur_num++;
            continue;
        }
        if (buffer[i] != '\n') {
            ans += buffer[i];
        }
    }
    return ans + '\0';
}


int main(int argc, char ** argv) {
    int status, sd;
    struct addrinfo hints, *res;
    struct sockaddr_storage sock_stor;

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
    int num_clients = 2;
    pollfd clients[MAX_CLIENTS];
    memset(clients, 0, sizeof(pollfd) * MAX_CLIENTS);
    my_buffer out_to_client[MAX_CLIENTS];

    // for accepting
    clients[0].fd = sd;
    clients[0].events = POLLIN;
    // from terminal
    clients[1].fd = 0;
    clients[1].events = POLLIN;
    map<string, vector<string> > history;
    for (int i = 2; i < argc; i++) {
        if ((status = getaddrinfo(NULL, argv[i], &hints, &res)) != 0) {
            perror("error getaddrfinfo");
            return 1;
        }
        if ((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
            perror("error socket");
            return 2;
        }
        int fd = connect(sd, res->ai_addr, res->ai_addrlen);
        if (fd != -1) {
            clients[num_clients].fd = sd;
            clients[num_clients].events = POLLIN;
            num_clients++;
            cout << "accepted " << sd << ", " << num_clients - 2 << " now" << endl;
        }
    }
    string key, value1, value2;
    while (1) {
        cout << "while" << endl;
        poll(clients, num_clients, -1);
        if (clients[0].revents & POLLIN) {
            socklen_t  addr_size = sizeof(sock_stor);
            clients[num_clients].fd = accept(sd, (struct sockaddr *) &sock_stor, &addr_size);
            clients[num_clients].events = POLLIN;
            out_to_client[num_clients].add("lyalya");
            clients[num_clients].events |= POLLOUT;
            num_clients++;
            cout << "accepted " << num_clients - 2 << " clients now" << endl;
        }
        
        if (clients[1].revents & POLLIN) {
            cout << "terminal" << endl;
            
        }
        
        for (int i = 2; i < num_clients; i++) {
            if (clients[i].revents & POLLOUT) {
                cout << "pollout " << i << endl;
                int cnt = write(clients[i].fd, out_to_client[i].buffer, out_to_client[i].len);
                cout << "write " << cnt << " bytes, " << out_to_client[i].len - cnt << " bytes left" << endl;
                memmove(out_to_client[i].buffer, out_to_client[i].buffer + cnt, out_to_client[i].len - cnt);
                out_to_client[i].len -= cnt;
                if (out_to_client[i].len == 0) {
                    clients[i].events &= ~POLLOUT;
                }
            }
            if (clients[i].revents & POLLIN) {
                cout << "pollin " << i << endl;
                char buf[50];
                int cnt = read(clients[i].fd, buf, 50);
                cout << "read " << cnt << " bytes" << endl;
                cout << "message: " << buf << endl;
                if (cnt == 0) {
                    clients[i].events &= ~POLLIN;
                }
            }
            
        }
    }
    
    close(sd);
    return 0;

}
