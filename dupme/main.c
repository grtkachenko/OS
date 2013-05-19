#include <stdlib.h>
#include <string.h>
#include <unistd.h>
const int IN = 0, OUT = 1;
void print_string_prefix(char* buffer, int count) {
    int num_ok = 0;
    while (num_ok < count) {
        int cur = write(OUT, buffer + num_ok, count - num_ok);
        if (cur > 0) {
            num_ok += cur;
        }
    }
}
void print_string_twice(char* buffer, int count) {
    print_string_prefix(buffer, count);
    print_string_prefix(buffer, count);
}
int main(int argc, char* argv[]) {
    if (argc < 2) {
        return 1;
    }
    enum state {
        PRINT, IGNORE
    } current_state;
    int k = atoi(argv[1]), len = 0;
    if (k < 1) {
        return 2;
    }
    char * buffer = (char *) malloc(k + 1);
    current_state = PRINT;

    while (1) {
        int read_res = read(IN, buffer + len, k + 1 - len);
        if (read_res == 0) {
            if (current_state == PRINT && len > 0 && len <= k) {
                buffer[len++] = '\n';
                print_string_twice(buffer, len);
            }
            break;
        }
        int left = len;
        int right = read_res + len;
        while (left < right) {
            if (buffer[left] == '\n') {
                left++;
                if (current_state == PRINT) {
                    print_string_twice(buffer, left);
                } else {
                    current_state = PRINT;
                }
                memmove(buffer, buffer + left, right - left);
                right = right - left;
                left = 0;
            } else {
                left++;
            }
        }
        if (left == k + 1) {
            len = 0;
            current_state = IGNORE;
        } else {
            len = left;
        }
    }
    free(buffer);
    return 0;
}
