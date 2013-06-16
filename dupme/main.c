#include <stdlib.h>
#include <string.h>
#include <unistd.h>
const int IN = 0, OUT = 1;

typedef enum {
    PRINT, IGNORE
} state;

void print_string_prefix(char* buffer, int left, int right) {
    int cur_left = left;
    while (cur_left < right) {
        int cur = write(OUT, buffer + cur_left, right - cur_left);
        if (cur > 0) {
            cur_left += cur;
        }
    }
}

void print_string_twice(char* buffer, int left, int right) {
    print_string_prefix(buffer, left, right);
    print_string_prefix(buffer, left, right);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        return 1;
    }
    int k = atoi(argv[1]), len = 0;
    if (k < 1) {
        return 2;
    }
    char * buffer = (char *) malloc(k + 1);
    if (buffer == NULL) {
        return 3;
    }
    state current_state = PRINT;

    while (1) {
        int read_res = read(IN, buffer + len, k + 1 - len);
        if (read_res == 0) {
            if (current_state == PRINT && len > 0 && len <= k) {
                buffer[len++] = '\n';
                print_string_twice(buffer, 0, len);
            }
            break;
        }
        int left = len, prev_left = 0;
        int right = read_res + len;
        while (left < right) {
            if (buffer[left] == '\n') {
                left++;
                if (current_state == PRINT) {
                    print_string_twice(buffer, prev_left, left);
                } else {
                    current_state = PRINT;
                }
                prev_left = left;
            } else {
                left++;
            }
        }
        if (left == k + 1 && prev_left == 0) {
            len = 0;
            current_state = IGNORE;
        } else {
            memmove(buffer, buffer + prev_left, right - prev_left);
            len = right - prev_left;
        }
    }
    free(buffer);
    return 0;
}
