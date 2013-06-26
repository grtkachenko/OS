#include <stdlib.h>
#include <stdio.h>
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

// -1 - doesn't have ans
int get_next_string(char * ans, char * buffer, int * len, int k) {
    state current_state = PRINT;
    while (1) {
        int read_res = read(IN, buffer + *len, k + 1 - *len);
        if (read_res == 0) {
            if (current_state == PRINT && *len > 0 && *len <= k) {
                buffer[(*len)++] = '\n';
                int tmp = *len;
                memcpy(ans, buffer, *len);
                *len = 0;
                return tmp;
            }
            break;
        }
        int i, was_moved = 0, right = read_res + *len;
        *len = right;
        for (i = 0; i < right; i++) {
            if (buffer[i] == '\n') {
                if (current_state == PRINT) {
                    memcpy(ans, buffer, i + 1);
                    *len -= i + 1;
                    memmove(buffer, buffer + i + 1, k + 1 - (i + 1)); 
                    return i + 1;
                } else {
                    current_state = PRINT;
                    *len -= i + 1;
                    memmove(buffer, buffer + i + 1, k + 1 - (i + 1)); 
                    was_moved = 1;
                    break;
                }
            }
        }
        if (was_moved) {
            continue;
        }
        if (right == k + 1) {
            *len = 0;
            current_state = IGNORE;
        } 
    }
    return -1;

}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        return 1;
    }
    int k = atoi(argv[1]), len = 0, res = 0;
    if (k < 1) {
        return 2;
    }
    char * buffer = (char *) malloc(k + 1);
    if (buffer == NULL) {
        return 3;
    }
    char * ans = (char *) malloc(k + 1);

    while (1) {
        if ((res = get_next_string(ans, buffer, &len, k)) == -1) {
            break;
        } 
        print_string_twice(ans, 0, res);
    }
    free(buffer);
    return 0;
}
