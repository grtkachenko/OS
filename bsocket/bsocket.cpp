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
#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <sstream>
using namespace std;
const int MAX_SIZE = 4096, MAX_CLIENTS = 100;
int pid;
int num_clients = 1;
void handler(int val) {
    (void)val;
    if (pid)
        kill(pid, SIGINT);
}
struct my_buffer {
    char * buffer;
    int len;
    int final_len = -1;
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

struct multihead_queue {
    vector<char> buffer;
    vector<char> how_much;
    int len = 0;
    int pos[MAX_CLIENTS];
    multihead_queue() {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            pos[i] = 0;
        }
    }
    void add(char * st, int size) {
        for (int i = 0; i < size; i++) {
            buffer.push_back(st[i]);
            how_much.push_back(num_clients - 1);
        }  
        len += size;
    }
    void add(string st) {
        for (char c : st) {
            buffer.push_back(c);
            how_much.push_back(num_clients - 1);
        }
        len += st.length();
    }
    void add_println() {
        buffer.push_back('\n');
        how_much.push_back(num_clients - 1);
        len++;
    }
    void add_msg_from(string st) {
        add("Message from ");
        add(st);
        add(" : ");
    }
    void update() {
        int right = -1;
        for (int i = 0; i < len; i++) {
            if (how_much[i] == 0) {
                right = i;
            }
        }
        if (right != -1) {
            vector<char> new_buffer, new_how_much;
            int new_len = 0;
            for (int i = right + 1; i < len; i++) {
                new_buffer.push_back(buffer[i]);
                new_how_much.push_back(how_much[i]);
                new_len++;
            }
            for (int i = 0; i < num_clients; i++) {
                pos[i] -= right + 1;
            }
            len = new_len;
            buffer = new_buffer;
            how_much = new_how_much;
        }
    }
};

string int_to_str(int val) {
    stringstream ss;
    ss << val;
    return ss.str();
}
int main() {
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
        pollfd clients[MAX_CLIENTS];
        my_buffer message_from[MAX_CLIENTS];
        string msg_cl[MAX_CLIENTS];

        // 0 - number, 1 - bytes

        clients[0].fd = sd;
        clients[0].events = POLLIN;
        multihead_queue q;
        
        while (1) {
            poll(clients, num_clients, -1);
            for (int i = 1; i < num_clients; i++) {
                if (clients[i].revents & POLLIN) {
                    int cnt = read(clients[i].fd, message_from[i].buffer + message_from[i].len, MAX_SIZE - message_from[i].len);
                    if (cnt != 0) {
                        message_from[i].len += cnt;
                        while (1) {
                            string val = "";
                            if (message_from[i].final_len == -1) {
                                for (int j = 0; j < message_from[i].len; j++) {
                                    if (message_from[i].buffer[j] == ' ') {
                                        message_from[i].final_len = stoi(val);
                                        memmove(message_from[i].buffer, message_from[i].buffer + j + 1, message_from[i].len - j - 1);
                                        message_from[i].len -= j + 1;
                                        break;     
                                    } else {
                                        val += message_from[i].buffer[j];
                                    }
                                } 
                            }
                            if (message_from[i].final_len != -1) {
                                if (message_from[i].len >= message_from[i].final_len) {
                                    q.add_msg_from(msg_cl[i]);
                                    q.add(message_from[i].buffer, message_from[i].final_len);
                                    q.add_println();
                                    for (int j = 1; j < num_clients; j++) {
                                        clients[j].events |= POLLOUT;
                                    } 
                                    memmove(message_from[i].buffer, message_from[i].buffer + message_from[i].final_len, message_from[i].len - message_from[i].final_len);
                                    message_from[i].len -= message_from[i].final_len;
                                    message_from[i].final_len = -1;
                                    continue;
                                } else {
                                    break;
                                }
                                
                            } else {
                                break;
                            }
                        }
                    } else {
                        clients[i].events &= ~POLLIN;
                    }
                }
                if (clients[i].revents & POLLOUT) {
                    int cnt = write(clients[i].fd, q.buffer.data() + q.pos[i], q.len - q.pos[i]);
                    if (cnt == 0) {
                        clients[i].events &= ~POLLOUT;
                    } else {
                        for (int j = q.pos[i]; j < q.pos[i] + cnt; j++) {
                            q.how_much[j]--;
                        }
                        q.pos[i] += cnt;
                        q.update();
                    }
                }
                
            }
            if (clients[0].revents & POLLIN) {
                sockaddr_in client;
                client.sin_family = AF_INET;
                socklen_t  addr_size = sizeof(client);
                clients[num_clients].fd = accept(sd, (struct sockaddr *) &client, &addr_size);
                for (int i = 0; i < q.len; i++) {
                    q.how_much[i]++;
                }
                msg_cl[num_clients] = inet_ntoa(client.sin_addr); 
                msg_cl[num_clients] += " " + int_to_str(client.sin_port) + " ";
                clients[num_clients].events = POLLIN;
                num_clients++;
            }
        }
        
        close(sd);
        return 0;
        
    }
}
