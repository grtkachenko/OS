#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
const int MAX_SIZE = 4095;
// Parsing string by spaces
char ** get_command(char * buffer, int len) {
    char ** ans = malloc(MAX_SIZE);
    int num = 0, i = 0, left = 0;
    for (i = 0; i < len; i++) {
       if (buffer[i] == ' ') {
           char * cur = malloc(i - left + 1);
           memcpy(cur, buffer + left, i - left);
           ans[num++] = cur;
           cur[i - left] = 0;
           left = i + 1;
        }
    }    
    char * cur = malloc(i - left + 1);
    memcpy(cur, buffer + left, i - left);
    ans[num++] = cur;
    cur[i - left] = 0;
    return ans;
}

// Debug function
void print_string_prefix(char* buffer, int count) {
    int num_ok = 0;
    while (num_ok < count) {
        int cur = write(1, buffer + num_ok, count - num_ok);
        if (cur > 0) {
            num_ok += cur;
        }
    }
    printf("\n");
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        return 1;
    }
    char * buffer = malloc(MAX_SIZE + 1);
    int num_files = 0, len = 0, last_size = 0;
    char ** files = malloc(MAX_SIZE);
    int in_file = open(argv[1], O_RDONLY);
    while (1) {
        int read_res = read(in_file, buffer + len, MAX_SIZE + 1 - len);
        if (read_res == 0) {
            if (len > 0 && len <= MAX_SIZE) {
                last_size = len;
                char * cur = malloc(len + 1);
                memcpy(cur, buffer, len);  
                cur[len] = 0;
                files[num_files++] = cur;
            }
            break;
        }
        int left = len;
        int right = read_res + len;
        while (left < right) {
            if (buffer[left] == '\n') {
                last_size = left;
                char * cur = malloc(left + 1);
                memcpy(cur, buffer, left);  
                files[num_files++] = cur;
                cur[left] = 0;
                left++;
                memmove(buffer, buffer + left, right - left);
                right = right - left;
                left = 0;
            } else {
                left++;
            }
        }
        if (left == MAX_SIZE + 1) {
            return 2;
        } else {
            len = left;
        }
    }
    close(in_file);
    char ** command = get_command(buffer, last_size);
    int fds[2];
    pipe(fds);
    if (!fork()) {
        //son
        close(fds[0]);
        int it; 
        char * current = malloc(MAX_SIZE);
        for (it = 0; it < num_files - 1; it++) {
            int newfds[2];
            if (!fork()) {
                int fdin = open(files[it], O_RDONLY);
                dup2(fdin, 0);
                dup2(fds[1], 1);
                execvp(argv[2], &argv[2]);
                exit(0);
            }
            wait(NULL);
        }
        close(fds[1]);
    } else {
        //parent
        wait(NULL);
        dup2(fds[0], 0);
        close(fds[0]);
        close(fds[1]);
        execvp(command[0], command); 
    }
    return 0;
}
