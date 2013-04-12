#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
const int IN = 0, OUT = 1;
char ** arr;
char delim = '\n';
int num_arr;
void print_ans(char * buffer, int count) {
    int num_ok = 0;
    while (num_ok < count) {
        int cur = write(OUT, buffer + num_ok, count - num_ok);
        if (cur > 0) {
            num_ok += cur;
        }
    }
}
void println() {
    char * println_s = malloc(1);
    println_s[0] = '\n';
    print_ans(println_s, 1);
}
void check_ans(char * buffer, int count) {
    int pipefd[2];
    pipe(pipefd);
    
    if (!fork()) {
        arr[num_arr] = buffer;
        dup2(pipefd[1], 1);
        execvp(arr[0], arr);
        exit(0);
    }
    int status = 0;
    wait(&status);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        print_ans(buffer, count);
        if (buffer[count - 1] != '\n') {
            println();
        }
    }
}
int main(int argc, char ** argv) {
    int num = 4095;
    int len = 0;
    int c;
    while ((c = getopt(argc, argv, "nzb:")) != -1) {
        if (c == 'n') {
            delim = '\n';
        }           
        if (c == 'z') {
            delim = '\0';
        }
        if (c == 'b') {
            num = atoi(optarg);
        }
    }
    char * buffer = malloc(num + 1);
    arr = (char **) malloc(argc - optind + 1);
    int i;
    for (i = optind; i < argc; i++) {
        if (strcmp(argv[i], "{}") == 0) {
            num_arr = i - optind;
        }
        arr[i - optind] = argv[i];
    }
    arr[argc - optind] = 0;
    while (1) {
        int read_res = read(IN, buffer + len, num + 1 - len);
        if (read_res == 0) {
            if (len == num) {
                return 2;
            }
            if (len > 0 && len < num) {
                check_ans(buffer, len);
            }
            break;
        }
        int left = len;
        int right = read_res + len;
        while (left < right) {
            if (buffer[left] == delim) {
                left++;
                check_ans(buffer, left);
                memmove(buffer, buffer + left, right - left);
                right = right - left;
                left = 0;
            } else {
                left++;
            }
        }
        if (left == num + 1) {
            return 1;
        } else {
            len = left;
        }

        
    }
    return 0;
}
