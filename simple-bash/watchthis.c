#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
const char * PREV = "/tmp/prev";
const char * CURRENT = "/tmp/current";

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
    mkfifo(PREV, S_IRUSR | S_IWUSR);
    mkfifo(CURRENT, S_IRUSR | S_IWUSR);

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
            int written = 0;
            while (written < used) {
                written += write(1, buffer + written, used - written); 
            }
            close(fds[0]);
            if (old_size > 0) {
                    
            }
            old_buffer = buffer;
            old_size = used;
            
        }
    }
    return 0;
}
