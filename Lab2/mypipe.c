#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 128

int main() {
    int pipefd[2];
    pid_t pid;
    char buffer[BUFFER_SIZE];

    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(1);
    }

    pid = fork();

    if (pid == -1) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) {
        close(pipefd[0]);
        const char *message = "hello";
        if (write(pipefd[1], message, strlen(message)) == -1) {
            perror("write failed");
            exit(1);
        }
        close(pipefd[1]);
        exit(0);
    } else {
        close(pipefd[1]);
        ssize_t bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1);
        if (bytesRead == -1) {
            perror("read failed");
            exit(1);
        }
        buffer[bytesRead] = '\0';
        printf("Received message: %s\n", buffer);
        close(pipefd[0]);
        wait(NULL);
    }

    return 0;
}
