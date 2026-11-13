#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const size_t MAX_LEN = 4096;

void error(const char *msg) {
    write(STDERR_FILENO, msg, strlen(msg));
    exit(EXIT_FAILURE);
}

ssize_t read_line(int fd, char **line, size_t *cap) {
    if (*line == NULL) {
        *cap = MAX_LEN;
        *line = malloc(*cap);
        if (*line == NULL)
            return -1;
    }
    size_t len = 0;
    while (true) {
        char c;
        ssize_t n = read(fd, &c, 1);
        if (n == -1)
            return -1;
        if (n == 0) {
            if (len == 0)
                return 0;
            break;
        }
        if (c == '\n')
            break;
        (*line)[len++] = c;
    }
    (*line)[len] = '\0';
    return (ssize_t)len;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        error("error: filename not provided\n");
    }

    int file = open(argv[1], O_RDONLY);
    if (file == -1)
        error("error: failed to open file\n");

    if (dup2(file, STDIN_FILENO) == -1)
        error("error: failed dup2 stdin\n");
    close(file);

    char *line = NULL;
    size_t cap = 0;
    float total_sum = 0.0;

    while (true) {
        ssize_t len = read_line(STDIN_FILENO, &line, &cap);
        if (len == -1)
            error("error: read failed\n");
        if (len == 0)
            break;

        char *ptr = line;
        while (*ptr) {
            while (*ptr && isspace((unsigned char)*ptr)) ptr++;
            if (!*ptr) break;

            char *endptr;
            float val = strtof(ptr, &endptr);
            if (ptr == endptr) {
                break;
            }
            total_sum += val;
            ptr = endptr;
        }
    }

    char out_buf[64];
    int out_len = snprintf(out_buf, sizeof(out_buf), "Sum: %f\n", total_sum);

    if (write(STDOUT_FILENO, out_buf, out_len) == -1)
        error("error: writing result failed\n");

    free(line);
    return 0;
}
