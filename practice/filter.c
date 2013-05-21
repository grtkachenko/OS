#include <unistd.h>
#include <string.h>
#include <stdlib.h>
const int IN = 0, OUT = 1;

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
    char endl = '\n';
    write(OUT, &endl, 1);
}

int is_ok_status(int status) {
    return WIFEXITED(status) && WEXITSTATUS(status) == 0; 
}

void check_ans(char ** arr_argv, int num_in_arr, char * buffer, int count) {
    int pipefd[2];
    pipe(pipefd);
    
    if (!fork()) {
        arr_argv[num_in_arr] = buffer;
        dup2(pipefd[1], 1);
        execvp(arr_argv[0], arr_argv);
        exit(0);
    }
    int status = 0;
    wait(&status);
    if (is_ok_status(status)) {
        print_ans(buffer, count);
        if (buffer[count - 1] != '\n') {
            println();
        }
    }
}

int main(int argc, char ** argv) {
    int num = 4095, len = 0, c, num_arr_argv;
    char delim = '\n';
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
    char * buffer = (char *) malloc(num + 1);
    if (buffer == NULL) {
        return 1;
    }
    char ** arr_argv = (char **) malloc(argc - optind + 1);
    if (arr_argv == NULL) {
        free(buffer);
        return 2;
    }
    int i;
    for (i = optind; i < argc; i++) {
        if (strcmp(argv[i], "{}") == 0) {
            num_arr_argv = i - optind;
        }
        arr_argv[i - optind] = argv[i];
    }
    arr_argv[argc - optind] = 0;
    while (1) {
        int read_res = read(IN, buffer + len, num + 1 - len);
        if (read_res < 0) {
            free(buffer);
            free(arr_argv);
            return 3;
        }
        if (read_res == 0) {
            if (len <= 0 || len == num) {
                free(buffer);
                free(arr_argv);
                return 4;
            }
            if (len > 0 && len < num) {
                check_ans(arr_argv, num_arr_argv, buffer, len);
            }
            break;
        }
        int left = len;
        int right = read_res + len;
        while (left < right) {
            if (buffer[left] == delim) {
                left++;
                check_ans(arr_argv, num_arr_argv, buffer, left);
                memmove(buffer, buffer + left, right - left);
                right = right - left;
                left = 0;
            } else {
                left++;
            }
        }
        if (left == num + 1) {
            free(buffer);
            free(arr_argv);
            return 5;
        } else {
            len = left;
        }
    }
    free(buffer);
    free(arr_argv);
    return 0;
}
