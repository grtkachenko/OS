#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
int main(int argc, char ** argv) {
    if (argc < 3) {
        return 1;
    }
    int interval = atoi(argv[1]);
    if (interval <= 0) {
        return 2;
    }
    while (1) {
        sleep(interval);
        int fds[2];
        pipe(fds);

        if (!fork()) {
            //son 
            dup2(fds[1], 1);
            close(fds[0]);
            close(fds[1]);
            execvp(argv[2], &argv[2]);
        } else {
            //parent
            wait(NULL);
            char * buffer = malloc(15);
            read(fds[0], buffer, 15); 
            close(fds[0]);
            close(fds[1]);
            //execl("/bin/ls", "ls", NULL);
            printf("%s\n", buffer);
        }
    }
    return 0;
}
