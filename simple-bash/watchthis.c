#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
const char * PREV = ".prev";
const char * CURRENT = ".current";
void write_to_fd(int fd, char * buffer, int size) {
    int written = 0;
    while (written < size) {
        written += write(fd, buffer + written, size - written);
    }
}
int main(int argc, char ** argv) {
    if (argc < 3) {
        return 1;
    }
    int buffer_size = 4096;
    int interval = atoi(argv[1]);
    if (interval <= 0) {
        return 2;
    }
    int old_size = 0;
    char * old_buffer = malloc(buffer_size);

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
            char * buffer = malloc(buffer_size);
            close(fds[1]);
            int read_result = 0, used = 0;
            while ((read_result = read(fds[0], buffer + used, buffer_size - used)) != 0) {
                used += read_result; 
            }
            close(fds[0]);
            //write_to_fd(1, buffer, used);
            if (old_size > 0) {
                int prev = open(PREV, O_WRONLY | O_CREAT | O_TRUNC);
                int current = open(CURRENT, O_WRONLY | O_CREAT | O_TRUNC);        
                write_to_fd(prev, old_buffer, old_size);
                write_to_fd(current, buffer, used);
                if (!fork()) {
                    close(prev);
                    close(current);
                    execlp("/usr/bin/diff", "diff", "-u", PREV, CURRENT, NULL);      
                }
                close(prev);
                close(current);
                        
            }
            old_buffer = buffer;
            old_size = used;
        }
    }
    return 0;
}
