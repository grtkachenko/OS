#include <stdio.h>
#include <string.h>
int main(int argc, char* argv[]) {
    int k;
    char *buffer;
    int cur_len;
    if (argc < 2) {
        return 1;
    }
    k = atoi(argv[1]);
    if (k < 1) {
        return 2;
    }
    buffer = (char *) malloc(k);
    cur_len = 0;
    
    int i;
    for (i = 2; i < argc; i++) {
        if (strlen(argv[i]) <= k) {
             printf("%s\n%s\n", argv[i], argv[i]);
        }
    }
    return 0;
}
