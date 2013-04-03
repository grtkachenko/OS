#include <stdlib.h>
int string_to_int(char* s) {
    int ans = 0;
    while (*s != 0) {
        ans = ans * 10 + (*s) - '0';
        s++;
    }
    return ans;
}
void print(int count) {
    int num_ok = 0;
    while (num_ok < count) {
        int cur = write(1, buffer + num_ok, count - num_ok);
        if (cur > 0) {
            num_ok += cur;
        }
    }
}
int main(int argc, char* argv[]) {
    enum state {
        PRINT, IGNOR
    } current_state;
    int k = string_to_int(argv[1]);
    char* buffer = malloc(k + 1);
    int len = 0;
    current_state = PRINT;
    while (1) {
        int read_res = read(0, buffer + len, k + 1 - len);
        if (read_res == 0) {
            if (len != 0 && len <= k && current_state == PRINT) {
                buffer[len] = '\n';
                len++;
                print(len);
                print(len);
            }
            break;
        }
    }
    return 0;
}
