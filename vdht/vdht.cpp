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
const string COLLISION = "Collision";
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
    if (ans.back() != '\0') {
        ans += '\0';
    }
    return ans;
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
            clients[num_clients].fd = sd;
            clients[num_clients].events = POLLIN;
            num_clients++;
        }
    }
    char * command = (char *) malloc(MAX_SIZE);
    string key, value1, value2;
    
    int cmd_len = 0;
    while (1) {
        poll(clients, num_clients, -1);
        
        for (int i = 2; i < num_clients; i++) {
            if (clients[i].revents & POLLOUT) {
                int cnt = write(clients[i].fd, out_to_client[i].buffer, out_to_client[i].len);
                memmove(out_to_client[i].buffer, out_to_client[i].buffer + cnt, out_to_client[i].len - cnt);
                out_to_client[i].len -= cnt;
                if (out_to_client[i].len == 0) {
                    clients[i].events = POLLIN;
                }
            }
            if (clients[i].revents & POLLIN) {
                char buffer[50];
                int cnt = read(clients[i].fd, buffer, 50);
                int cmd_len = cnt;
                if (cnt != 0) {
                    int status = parse(buffer, cnt);
                    if (status != -1) {
                        key = get_next(buffer, cmd_len, ' ', 1);
                        if (status == 0) {
                            value1 = get_next(buffer, cmd_len, ' ', 2);
                            value2 = get_next(buffer, cmd_len, ' ', 3);
                            if (history[key].empty() || history[key].back().empty()) {
                                history[key].push_back(value2);
                                for (int j = 2; j < num_clients; j++) {
                                    clients[j].events |= POLLOUT;
                                    out_to_client[j].add(buffer);
                                }
                            } else {
                                if ((int) history[key].size() != 1 || history[key].back() != value2) {
                                    bool have_collision = true;
                                    bool have_element = false;
                                    for (int i = 0; i < (int) history[key].size(); i++) {
                                        if (history[key][i] == value1) {
                                            have_element = true;
                                            if (i + 1 != (int) history[key].size()) {
                                                if (history[key][i + 1] != value2) {
                                                    have_collision = true;
                                                } else {
                                                    have_collision = false;
                                                }
                                            } else {
                                                history[key].push_back(value2);
                                                for (int j = 2; j < num_clients; j++) {
                                                    clients[j].events |= POLLOUT;
                                                    out_to_client[j].add(buffer);
                                                }
                                                have_collision = false;
                                            }
                                            break;
                                        } 
                                        if (history[key][i] == value2) {
                                            if (i > 0 && history[key][i - 1] == "") {
                                                have_collision = false;
                                                break;
                                            }
                                        }
                                    }
                                    have_collision &= !have_element;
                                    if (have_collision) {
                                        history[key].push_back("");
                                        for (int i = 2; i < num_clients; i++) {
                                            clients[i].events |= POLLOUT;
                                            out_to_client[i].add("collision ");
                                            out_to_client[i].add(key);
                                            out_to_client[i].add("\n");
                                        }
                                    }
                                }
                            }
                        }
                        if (status == 1) {
                            if (history[key].empty()) {
                                // for debug
                                cout << "empty history" << endl;
                            } else
                            if (!history[key].back().empty()) {
                                history[key].push_back("");
                                for (int i = 2; i < num_clients; i++) {
                                    clients[i].events |= POLLOUT;
                                    out_to_client[i].add("collision ");
                                    out_to_client[i].add(key);
                                    out_to_client[i].add("\n");
                                }
                            }

                        }
                        if (status == 2) {
                            for (string st : history[key]) {
                                if (st.empty()) {
                                    cout << COLLISION << endl;
                                    continue;
                                }
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
                    if (history[key].empty() || history[key].back().empty()) {
                        history[key].push_back(value2);
                        for (int j = 2; j < num_clients; j++) {
                            clients[j].events |= POLLOUT;
                            out_to_client[j].add(command);
                        }
                    } else {
                        bool have_collision = true;
                        bool have_element = false;
                        
                        for (int i = 0; i < (int) history[key].size(); i++) {
                            if (history[key][i] == value1) {
                                have_element = true;
                                if (i + 1 != (int) history[key].size()) {
                                    if (history[key][i + 1] != value2) {
                                        have_collision = true;
                                    } else {
                                        have_collision = false;
                                    }
                                } else {
                                    history[key].push_back(value2);
                                    for (int j = 2; j < num_clients; j++) {
                                        clients[j].events |= POLLOUT;
                                        out_to_client[j].add(command);
                                    }
                                    have_collision = false;
                                }
                                break;
                            } 
                            if (history[key][i] == value2) {
                                if (i > 0 && history[key][i - 1] == "") {
                                    have_collision = false;
                                    break;
                                }
                            }
                        }
                        have_collision &= !have_element;
                        if (have_collision) {
                            history[key].push_back("");
                            for (int j = 2; j < num_clients; j++) {
                                clients[j].events |= POLLOUT;
                                out_to_client[j].add("collision ");
                                out_to_client[j].add(key);
                                out_to_client[j].add("\n");
                            }
                        }
                    }
                }
                if (status == 2) {
                    for (string st : history[key]) {
                        if (st.empty()) {
                            cout << COLLISION << endl;
                            continue;
                        }
                        cout << st << endl;
                    }
                }
                cmd_len = 0;
                key = value1 = value2 = "";
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
