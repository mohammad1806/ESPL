#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int pipefd[2]; // Pipe file descriptors
    pid_t child1, child2;

    fprintf(stderr, "(parent_process>forking…)\n");
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Fork first child
    child1 = fork();
    if (child1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child1 == 0) { // First child process
        fprintf(stderr, "(child1>redirecting stdout to the write end of the pipe…)\n");
        close(STDOUT_FILENO);
        dup(pipefd[1]); // Duplicate write end of the pipe to stdout
        close(pipefd[1]);
        close(pipefd[0]); // Close unused read end of the pipe

        fprintf(stderr, "(child1>going to execute cmd: ls -l)\n");
        char *args[] = {"in.txt", 0}; // Arguments for ls -l, ending with 0
        execvp("in.txt", args);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "(parent_process>created process with id: %d)\n", child1);

    close(pipefd[1]); // Close write end of the pipe
    fprintf(stderr, "(parent_process>closing the write end of the pipe…)\n");

    // Fork second child
    child2 = fork();
    if (child2 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child2 == 0) { // Second child process
        fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe…)\n");
        close(STDIN_FILENO);
        dup(pipefd[0]); // Duplicate read end of the pipe to stdin
        close(pipefd[0]);

        fprintf(stderr, "(child2>going to execute cmd: tail -n 2)\n");
        char *args[] = {"tail", "-n", "2", 0}; // Arguments for tail -n 2, ending with 0
        execvp("tail", args);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "(parent_process>created process with id: %d)\n", child2);

    close(pipefd[0]); // Close read end of the pipe
    fprintf(stderr, "(parent_process>closing the read end of the pipe…)\n");

    fprintf(stderr, "(parent_process>waiting for child processes to terminate…)\n");
    waitpid(child1, NULL, 0); // Wait for child1 to terminate
    waitpid(child2, NULL, 0); // Wait for child2 to terminate

    fprintf(stderr, "(parent_process>exiting…)\n");
    return 0;
}
