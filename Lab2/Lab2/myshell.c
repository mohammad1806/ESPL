#include <stdio.h>
#include "LineParser.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

int toWait = 1;

void handleProcessCommand(cmdLine *pCmdLine) {
    if (pCmdLine->argCount < 2) {
        fprintf(stderr, "Error: Process ID missing\n");
        return;
    }

    pid_t pid = atoi(pCmdLine->arguments[1]);

    if (strcmp(pCmdLine->arguments[0], "stop") == 0) {
        if (kill(pid, SIGSTOP) == -1) {
            perror("Error stopping process");
        } else {
            printf("Process %d stopped\n", pid);
        }
    } else if (strcmp(pCmdLine->arguments[0], "wake") == 0) {
        if (kill(pid, SIGCONT) == -1) {
            perror("Error waking process");
        } else {
            printf("Process %d continued\n", pid);
        }
    } else if (strcmp(pCmdLine->arguments[0], "term") == 0) {
        if (kill(pid, SIGINT) == -1) {
            perror("Error terminating process");
        } else {
            printf("Process %d terminated\n", pid);
        }
    }
}

void execute(cmdLine *pCmdLine, int debug) {
    if (strcmp(pCmdLine->arguments[0], "cd") == 0) {
        if (pCmdLine->argCount < 2) {
            fprintf(stderr, "cd: missing argument\n");
        } else {
            if (chdir(pCmdLine->arguments[1]) != 0) {
                perror("cd failed");
            }
        }
    } else if (strcmp(pCmdLine->arguments[0], "stop") == 0 || strcmp(pCmdLine->arguments[0], "wake") == 0 || strcmp(pCmdLine->arguments[0], "term") == 0) {
        handleProcessCommand(pCmdLine);
    } else {
        int pid = fork();

        if (pid == 0) {
            char path[2048] = "/bin/";
            strcat(path, pCmdLine->arguments[0]);

            if (debug) {
                fprintf(stderr, "Child executing: %s with argument count: %d\n", path, pCmdLine->argCount);
                fprintf(stderr , "PID is 0\n");
            }

            if(pCmdLine->outputRedirect != NULL) {
                int fd = open(pCmdLine->outputRedirect, O_WRONLY | O_CREAT | O_TRUNC);
                if (fd == -1) {
                    perror("Error opening file");
                }
                if (dup2(fd, STDOUT_FILENO) == -1) {
                    perror("Error redirecting stdin");
                }
                close(fd);
            }

            if(pCmdLine->inputRedirect != NULL) {
                int fd = open(pCmdLine->inputRedirect, O_RDONLY );
                if (fd == -1) {
                    perror("Error opening file");
                }
                if (dup2(fd, STDIN_FILENO) == -1) {
                    perror("Error redirecting stdin");
                }
                close(fd);
            }

            int res = execvp(pCmdLine->arguments[0], pCmdLine->arguments);

            if (res == -1) {
                perror("execvp failed");
            }
            exit(1);
        } else if (pid > 0 ) {
            if(toWait)
                waitpid(pid, NULL, 0);
            else if(debug)
                printf("we will not wait until finishing the process\n");
            if (debug) { 
                fprintf(stderr , "PID for this process is %d\n",pid);
            }
        } else {
            perror("fork failed");
        }
    }
}

int main(int argc, char** argv) {
    int debug = 0;

    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        debug = 1;
    }

    while (1) {
        char dir[2048];
        getcwd(dir, sizeof(dir));
        printf("%s > ", dir);

        char input[2048];
        fgets(input, sizeof(input), stdin);

        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "quit") == 0) {
            printf("Exiting shell...\n");
            break;
        }
        if(input[strlen(input)-1]=='&'){
            toWait=0;
        } else {
            toWait=1;
        }

        cmdLine *parsedInput = parseCmdLines(input);

        execute(parsedInput, debug);
    }

    return 0;
}
