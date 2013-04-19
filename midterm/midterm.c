#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
const char * MAIN_FILE = "main_file";
int IN = 0, OUT = 1;
char ** get_command(char * buffer, int len) {
    char ** ans = malloc(4096);
    int num = 0, i = 0, left = 0;
    for (i = 0; i < len; i++) {
       if (buffer[i] == ' ') {
           int right = i - 1;
           char * cur = malloc(right - left + 2);
           memcpy(cur, buffer + left, right - left + 1);
           ans[num++] = cur;
           cur[right - left + 1] = 0;
           left = i + 1;
        }
    }    
    int right = i - 1;
    char * cur = malloc(right - left + 2);
    memcpy(cur, buffer + left, right - left + 1);
    ans[num++] = cur;
    cur[right - left + 1] = 0;
    return ans;
}
void print_string_prefix(char* buffer, int count) {
    int num_ok = 0;
    while (num_ok < count) {
        int cur = write(OUT, buffer + num_ok, count - num_ok);
        if (cur > 0) {
            num_ok += cur;
        }
    }
    printf("\n");
}

int main(int argc, char* argv[]) {
    if (argc < 1) {
        return 1;
    }
    int buffer_size = 4096;
    char * buffer = malloc(buffer_size + 1);
    int len = 0;
    int last_size = 0;
    char ** files = malloc(4096);
    int num_files = 0;
    IN = open(MAIN_FILE, O_RDONLY);
    while (1) {
        int read_res = read(IN, buffer + len, buffer_size + 1 - len);
        if (read_res == 0) {
            if (len > 0 && len <= buffer_size) {
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
        if (left == buffer_size + 1) {
            return 2;
        } else {
            len = left;
        }
    }
    close(IN);
    char ** command = get_command(buffer, last_size);
    int fds[2];
    pipe(fds);
    if (!fork()) {
        //son
        close(fds[0]);
        int it; 
        char * current = malloc(buffer_size);
        for (it = 0; it < num_files - 1; it++) {
            int fdin = open(files[it], O_RDONLY);
            int read_res, used = 0;
            
            while ((read_res = read(fdin, current + used, buffer_size - used)) != 0) {
                used += read_res;
            } 
            int written = 0;
            while (written < used) {
                written += write(fds[1], current + written, used - written);
            }

            close(fdin);
        }
        close(fds[1]);
    } else {
        //parent
        wait(NULL);
        dup2(fds[0], 0);
        close(fds[0]);
        close(fds[1]);
        execvp(command[0], command);
        exit(0);
    }
    return 0;
}
