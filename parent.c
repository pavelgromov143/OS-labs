#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>

static char CHILD_NAME[] = "child";
static char CHILD_PATH[] = "./child";
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

int main() {
    char *filename = NULL;
    size_t cap = 0;

    const char hint[] = "Enter filename: ";
    write(STDERR_FILENO, hint, sizeof(hint) - 1);
    if (read_line(STDIN_FILENO, &filename, &cap) <= 0) {
        error("error: no filename\n");
    }

    int p1[2];
    if (pipe(p1) == -1) {
        error("error: failed to create pipe\n");
    }

    const pid_t child = fork();
    if (child == -1) {
        error("error: failed to spawn new process\n");
    }

    if (child == 0) {
        close(p1[0]);

        if (dup2(p1[1], STDOUT_FILENO) == -1) {
            error("error: failed dup2 stdout\n");
        }
        close(p1[1]);

        char *const args[] = {CHILD_NAME, filename, NULL};
        int32_t status = execv(CHILD_PATH, args);
        if (status == -1) {
            error("error: failed to exec into new executable image\n");
        }
    }

    close(p1[1]);

    char buf[MAX_LEN];
    ssize_t n;
    while ((n = read(p1[0], buf, sizeof(buf))) > 0) {
        if (write(STDOUT_FILENO, buf, n) == -1) {
            error("error: failed to write to stdout\n");
        }
    }
    if (n == -1) {
        error("error: reading from pipe failed\n");
    }

    close(p1[0]);

    wait(NULL);
    free(filename);
    return 0;
}