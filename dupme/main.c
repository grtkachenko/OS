#include <stdlib.h>
#include <unistd.h>
const int IN = 0, OUT = 1;
int string_to_int(char* s) {
    int ans = 0;
    while (*s != 0) {
        ans = ans * 10 + (*s) - '0';
        s++;
    }
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
}
int main(int argc, char* argv[]) {
    enum state {
        PRINT, IGNORE
    } current_state;
    char* buffer;
    int k = string_to_int(argv[1]);
    buffer = malloc(k + 1);
    int len = 0;
    current_state = PRINT;

    while (1) {
        int read_res = read(IN, buffer + len, k + 1 - len);
//        print(k + 1);
//        printf(" %d", len);
        if (read_res == 0) {
            if (current_state == PRINT && len > 0 && len <= k) {
                buffer[len] = '\n';
                len++;
                print_string_prefix(buffer, len);
                print_string_prefix(buffer, len);
            }
            break;
        }
        int left = len;
        int right = read_res + len;
        while (left < right) {
            if (buffer[left] == '\n') {
                if (current_state == PRINT) {
                    left++;
                    print_string_prefix(buffer, left);
                    print_string_prefix(buffer, left);
                } else {
                    left++;
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
