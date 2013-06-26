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
#include <iostream>
using namespace std;
const int MAX_SIZE = 4096, MAX_CLIENTS = 100;
const string COLLISION = "\0";


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
            printf("Connected!\n");
            clients[num_clients].fd = fd;
            clients[num_clients].events = POLLIN;
            num_clients++;
        }
    }
    char * command = (char *) malloc(MAX_SIZE);
    string key, value1, value2;
    
    int cmd_len = 0;
    while (1) {
        poll(clients, num_clients, -1);
        if (clients[0].revents & POLLIN) {
            socklen_t  addr_size = sizeof(sock_stor);
            clients[num_clients].fd = accept(sd, (struct sockaddr *) &sock_stor, &addr_size);
            clients[num_clients].events = POLLIN;
            num_clients++;
            printf("New client connected\n");
        }
        
        for (int i = 2; i < num_clients; i++) {
            if (clients[i].revents & POLLOUT) {
                int cnt = write(clients[i].fd, out_to_client[i].buffer, out_to_client[i].len);
                memmove(out_to_client[i].buffer, out_to_client[i].buffer + cnt, out_to_client[i].len - cnt);
                out_to_client[i].len -= cnt;
                cout << out_to_client[i].buffer << endl;
                if (out_to_client[i].len == 0) {
                    clients[i].events = POLLIN;
                }
            }
            if (clients[i].revents & POLLIN) {
                char buffer[50];
                int cnt = read(clients[i].fd, buffer, 50);
                if (cnt != 0) {
                    int status = parse(buffer, cnt);
                    if (status != -1) {
                        key = get_next(buffer, cnt, ' ', 1);
                        if (status == 0) {
                            value1 = get_next(buffer, cnt, ' ', 2);
                            value2 = get_next(buffer, cnt, ' ', 3);
                            if (history[key].empty()) {
                                history[key].push_back(value2);
                                for (int j = 2; j < num_clients; j++) {
                                    clients[j].events |= POLLOUT;
                                    for (int kk = 0; kk < cnt; kk++) {
                                        out_to_client[j].buffer[out_to_client[j].size++] = buffer[kk];
                                    }
                                }
                            } else {
                                for (int i = 0; i < (int) history[key].size(); i++) {
                                    if (history[key][i] == value1) {
                                        if (i + 1 != (int) history[key].size()) {
                                            if (history[key][i + 1] != value2) {
                                                for (int j = 2; j < num_clients; j++) {
                                                    clients[j].events |= POLLOUT;
                                                    out_to_client[j].add("collision ");
                                                    out_to_client[j].add(key);
                                                }
                                            }
                                        } else {
                                            history[key].push_back(value2);
                                            for (int j = 2; j < num_clients; j++) {
                                                clients[j].events |= POLLOUT;
                                                out_to_client[j].add(buffer);
                                            }
                                        }
                                        break;
                                    } 
                                }
                            }
                        }
                        if (status == 2) {
                            for (string st : history[key]) {
                                cout << st << endl;
                            }
                        }
                        cmd_len = 0;
                        key = value1 = value2 = "";
                    }
                }
            }
            
        }
        if (clients[1].revents & POLLIN) {
            int cnt = read(0, command + cmd_len, MAX_SIZE - cmd_len);
            cmd_len += cnt;
            int status;
            if ((status = parse(command, cnt)) != -1) {
                key = get_next(command, cmd_len, ' ', 1);
                if (status == 0) {
                    value1 = get_next(command, cmd_len, ' ', 2);
                    value2 = get_next(command, cmd_len, ' ', 3);
                    if (history[key].empty()) {
                        history[key].push_back(value2);
                        for (int j = 2; j < num_clients; j++) {
                            clients[j].events |= POLLOUT;
                            out_to_client[j].add(command);
                        }
                    } else {
                        for (int i = 0; i < (int) history[key].size(); i++) {
                            if (history[key][i] == value1) {
                                if (i + 1 != (int) history[key].size()) {
                                    if (history[key][i + 1] != value2) {
                                        for (int j = 2; j < num_clients; j++) {
                                            clients[j].events |= POLLOUT;
                                            out_to_client[j].add("collision ");
                                            out_to_client[j].add(key);
                                        }
                                    }
                                } else {
                                    history[key].push_back(value2);
                                    for (int j = 2; j < num_clients; j++) {
                                        clients[j].events |= POLLOUT;
                                        out_to_client[j].add(command);
                                    }
                                }
                                break;
                            } 
                        }
                    }
                }
                if (status == 2) {
                    for (string st : history[key]) {
                        cout << st << endl;
                    }
                    for (int j = 2; j < num_clients; j++) {
                        clients[j].events |= POLLOUT;
                        out_to_client[j].add(command);
                    }
                }
                cmd_len = 0;
                key = value1 = value2 = "";
            } 
        }
        
        
    }
    
    close(sd);
    return 0;

}
