#include <stdio.h>
#include "lineParser.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <ctype.h>
#include <linux/wait.h>

typedef struct process
{
    cmdLine *cmd;         /* the parsed command line*/
    pid_t pid;            /* the process id that is running the command*/
    int status;           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next; /* next process in chain */
} process;
int toWait = 1;

#define TERMINATED -1
#define RUNNING 1
#define SUSPENDED 0

process *first = NULL;
process **PL = &first;
int PLen = -1;

#define HISTLEN 10
char *history[HISTLEN];
int historyIndex = 0;
int oldestIndex = 0;

void addCmdToHistory(char *command)
{

    char *newCommand = strdup(command);
    if (!newCommand)
    {
        perror("strdup");
        return;
    }

    if (history[historyIndex] != NULL)
    {
        free(history[historyIndex]);
    }

    history[historyIndex] = newCommand;

    historyIndex = (historyIndex + 1) % HISTLEN;
    if (historyIndex == oldestIndex)
    {
        oldestIndex = (oldestIndex + 1) % HISTLEN;
    }
}

char *lastCmd()
{

    if (historyIndex == oldestIndex)
    {
        printf("History is empty! No previous command.\n");
        return NULL;
    }

    int lastCommandIndex = (historyIndex - 1 + HISTLEN) % HISTLEN;
    return history[lastCommandIndex];
}

char *nthCmd(int n)
{

    if (n < 1 || n > HISTLEN || historyIndex == oldestIndex)
    {
        printf("Invalid command number or history is empty.\n");
        return NULL;
    }

    // Calculate the index of the nth command from the oldest
    int commandIndex = (oldestIndex + n - 1) % HISTLEN;

    // Ensure the requested command exists
    if (commandIndex == historyIndex || history[commandIndex] == NULL)
    {
        printf("Command not found in history.\n");
        return NULL;
    }

    return history[commandIndex];
}

void printHistory()
{
    // Iterate through the history circular buffer
    int currentIndex = oldestIndex;
    int commandNumber = 1;

    while (currentIndex != historyIndex)
    {
        printf("%d) %s\n", commandNumber, history[currentIndex]);
        currentIndex = (currentIndex + 1) % HISTLEN;
        commandNumber++;
    }
}

cmdLine *copyCmdLine(const cmdLine *pCmdLine)
{
    if (pCmdLine == NULL)
        return NULL;
    cmdLine *copy = (cmdLine *)malloc(sizeof(cmdLine));
    memcpy(copy, pCmdLine, sizeof(cmdLine));
    copy->inputRedirect = pCmdLine->inputRedirect ? strdup(pCmdLine->inputRedirect) : NULL;
    copy->outputRedirect = pCmdLine->outputRedirect ? strdup(pCmdLine->outputRedirect) : NULL;
    copy->next = NULL;
    for (int i = 0; i < pCmdLine->argCount; i++)
    {
        copy->arguments[i] = strdup(pCmdLine->arguments[i]);
    }
    return copy;
}

void addProcess(process **process_list, cmdLine *cmd, pid_t pid)
{
    process *new_process = (process *)malloc(sizeof(process));
    new_process->cmd = copyCmdLine(cmd);
    new_process->pid = pid;
    new_process->status = RUNNING;
    new_process->next = NULL;
    if (*PL == NULL)
    {
        *process_list = new_process;
    }
    else
    {
        new_process->next = *process_list;
        *process_list = new_process;
    }
    PLen++;
}

void freeProcessList(process **process_list)
{
    process *curr_process = *process_list;
    while (curr_process != NULL)
    {
        process *next_process = curr_process->next;
        freeCmdLines(curr_process->cmd);
        free(curr_process);
        curr_process = next_process;
    }
    *process_list = NULL;
}

void updateProcessStatus(process *process_list, int pid, int status)
{
    if (process_list == NULL)
    {
        return;
    }
    process *temp_process_list = process_list;
    while (temp_process_list != NULL)
    {
        if (temp_process_list->pid == pid)
        {
            temp_process_list->status = status;
            return;
        }
        temp_process_list = temp_process_list->next;
    }
}

void updateProcessList(process **process_list)
{
    if (process_list == NULL)
    {
        return;
    }
    process *temp_process_list = *process_list;
    while (temp_process_list != NULL)
    {
        int status;
        int res = waitpid(temp_process_list->pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
        if (res == -1)
        {
            temp_process_list = temp_process_list->next;
            continue;
        }
        if (res == temp_process_list->pid)
        {
            if (WIFSIGNALED(status) || WIFEXITED(status))
            {
                updateProcessStatus(*process_list, temp_process_list->pid, TERMINATED);
            }
            else if (WIFSTOPPED(status))
            {
                updateProcessStatus(*process_list, temp_process_list->pid, SUSPENDED);
            }
            else if (WIFCONTINUED(status))
            {
                updateProcessStatus(*process_list, temp_process_list->pid, RUNNING);
            }
        }
        temp_process_list = temp_process_list->next;
    }
}

void deleteProcess(process **process_list, process *p)
{
    if (*process_list == NULL)
        return;
    if (*process_list == p)
    {
        *process_list = (*process_list)->next;
        free(p);
        return;
    }
    process *prev = *process_list;
    while (prev->next != NULL && prev->next != p)
    {
        prev = prev->next;
    }
    if (prev->next != NULL)
    {
        prev->next = prev->next->next;
        free(p);
    }
}

char *concatWithSpaces(char *arr[], int size)
{
    // Calculate the total length needed for the concatenated string
    int total_length = 0;
    for (int i = 0; i < size; i++)
        total_length += strlen(arr[i]) + 1;

    // Allocate memory for the result and initialize it as an empty string
    char *result = (char *)malloc(total_length * sizeof(char));
    if (!result)
        return NULL;

    result[0] = '\0'; // Start with an empty string

    // Concatenate all strings with a space in between
    for (int i = 0; i < size; i++)
    {
        strcat(result, arr[i]);
        if (i < size - 1)
            strcat(result, " "); // Add space between strings
    }

    return result;
}

void printProcessList(process **process_list)
{
    if (process_list == NULL)
    {
        printf("Error: process_list is NULL\n");
        return;
    }
    updateProcessList(process_list);
    int idx = PLen;
    process *temp_process_list = *process_list;
    printf("%-12s %-12s %-12s %-12s\n", "Index", "PID", "Command", "STATUS");
    while (temp_process_list != NULL)
    {

        printf("%-12d %-12d %-12s ", idx, temp_process_list->pid, concatWithSpaces(temp_process_list->cmd->arguments, temp_process_list->cmd->argCount));
        switch (temp_process_list->status)
        {
        case RUNNING:
            printf("%-12s\n", "Running");
            break;
        case SUSPENDED:
            printf("%-12s\n", "Suspend");
            break;
        case TERMINATED:
            printf("%-12s\n", "Terminated");
            deleteProcess(process_list, temp_process_list);
            break;
        }
        temp_process_list = temp_process_list->next;
        idx--;
    }
}

bool isForm_N(const char *str)
{
    if (str[0] != '!')
        return false;
    for (int i = 1; str[i] != '\0'; i++)
    {
        if (!isdigit(str[i]))
            return false;
    }
    return true;
}

void handleProcessCommand(cmdLine *pCmdLine)
{
    if (pCmdLine->argCount < 2)
    {
        fprintf(stderr, "Error: Process ID missing\n");
        return;
    }

    pid_t pid = atoi(pCmdLine->arguments[1]);

    if (strcmp(pCmdLine->arguments[0], "stop") == 0)
    {
        if (kill(pid, SIGSTOP) == -1)
        {
            perror("Error stopping process");
        }
        else
        {
            printf("Process %d stopped\n", pid);
        }
    }
    else if (strcmp(pCmdLine->arguments[0], "wake") == 0)
    {
        if (kill(pid, SIGCONT) == -1)
        {
            perror("Error waking process");
        }
        else
        {
            printf("Process %d continued\n", pid);
        }
    }
    else if (strcmp(pCmdLine->arguments[0], "term") == 0)
    {
        if (kill(pid, SIGINT) == -1)
        {
            perror("Error terminating process");
        }
        else
        {
            printf("Process %d terminated\n", pid);
        }
    }
}

void execute(cmdLine *pCmdLine, int debug)
{
    if (strcmp(pCmdLine->arguments[0], "cd") == 0)
    {
        if (pCmdLine->argCount < 2)
        {
            fprintf(stderr, "cd: missing argument\n");
        }
        else
        {
            if (chdir(pCmdLine->arguments[1]) != 0)
            {
                perror("cd failed");
            }
        }
    }
    else if (strcmp(pCmdLine->arguments[0], "procs") == 0)
    {
        printProcessList(PL);
    }
    else if (strcmp(pCmdLine->arguments[0], "history") == 0)
    {
        printHistory();
    }
    else if (strcmp(pCmdLine->arguments[0], "stop") == 0 || strcmp(pCmdLine->arguments[0], "wake") == 0 || strcmp(pCmdLine->arguments[0], "term") == 0)
    {
        handleProcessCommand(pCmdLine);
    }
    else
    {
        int pid = fork();

        if (pid == 0)
        {
            if (strcmp(pCmdLine->arguments[0], "!!") == 0)
            {
                char *cmd = lastCmd();
                pCmdLine = parseCmdLines(cmd);
                //!! is added to history
            }
            else if (isForm_N(pCmdLine->arguments[0]))
            {
                int n = atoi(pCmdLine->arguments[0] + 1);
                char *cmd = nthCmd(n);
                pCmdLine = parseCmdLines(cmd);
            }

            char path[2048] = "/bin/";
            strcat(path, pCmdLine->arguments[0]);

            if (debug)
            {
                fprintf(stderr, "Child executing: %s with argument count: %d\n", path, pCmdLine->argCount);
                fprintf(stderr, "PID is 0\n");
            }

            if (pCmdLine->outputRedirect != NULL)
            {
                int fd = open(pCmdLine->outputRedirect, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd == -1)
                {
                    perror("Error opening file");
                }
                if (dup2(fd, STDOUT_FILENO) == -1)
                {
                    perror("Error redirecting stdout");
                }
                close(fd);
            }

            if (pCmdLine->inputRedirect != NULL)
            {
                int fd = open(pCmdLine->inputRedirect, O_RDONLY);
                if (fd == -1)
                {
                    perror("Error opening file");
                }
                if (dup2(fd, STDIN_FILENO) == -1)
                {
                    perror("Error redirecting stdin");
                }
                close(fd);
            }
            
            int res = execvp(pCmdLine->arguments[0], pCmdLine->arguments);
            if (res == -1)
            {
                perror("execvp failed");
            }
            exit(1);
        }
        else if (pid > 0)
        {
            if (toWait)
                waitpid(pid, NULL, 0);
            else if (debug)
                printf("we will not wait until finishing the process\n");
            if (debug)
            {
                fprintf(stderr, "PID for this process is %d\n", pid);
            }
            addProcess(PL, pCmdLine, pid);
        }
        else
        {
            perror("fork failed");
        }
    }
}
// -------------------------------------------------------------------------------------------------->>
/// @brief
/// @param argc
/// @param argv
/// @return
int main(int argc, char **argv)
{
    int debug = 0;

    if (argc > 1 && strcmp(argv[1], "-d") == 0)
    {
        debug = 1;
    }
    // printf("process sixe is: %d", sizeof(process));
    while (1)
    {
        char dir[2048];
        getcwd(dir, sizeof(dir));
        printf("%s > ", dir);

        char input[2048];
        fgets(input, sizeof(input), stdin);

        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "quit") == 0)
        {
            printf("Exiting shell...\n");
            freeProcessList(PL);
            break;
        }
        if (input[strlen(input) - 1] == '&')
        {
            toWait = 1;
        }
        else
        {
            toWait = 0;
        }

        cmdLine *parsedInput = parseCmdLines(input);
        if (strcmp(parsedInput->arguments[0], "history") != 0 && strcmp(parsedInput->arguments[0], "!!") != 0 && !isForm_N(parsedInput->arguments[0]))
        {
            addCmdToHistory(concatWithSpaces(parsedInput->arguments, parsedInput->argCount));
        }

        if (parsedInput->next != NULL)
        {

            // meaning there is pipe
            int pipefd[2];
            pid_t childp1, childp2;
            if (pipe(pipefd) == -1)
            {
                perror("Error in making pipe");
                exit(EXIT_FAILURE);
            }

            childp1 = fork();

            if (childp1 == -1)
            {
                perror("Error in forking child1");
                exit(EXIT_FAILURE);
            }

            if (childp1 == 0)
            {
                close(STDOUT_FILENO);
                if (dup(pipefd[1]) == -1)
                {
                    perror("dup");
                    exit(EXIT_FAILURE);
                }
                close(pipefd[1]);

                if (parsedInput->inputRedirect != NULL)
                {
                    int fd = open(parsedInput->inputRedirect, O_RDONLY);
                    if (fd == -1)
                    {
                        perror("Error opening file");
                    }
                    if (dup2(fd, STDIN_FILENO) == -1)
                    {
                        perror("Error redirecting stdin");
                    }
                    close(fd);
                }
                if (execvp(parsedInput->arguments[0], parsedInput->arguments) == -1)
                {
                    perror("error in execv");
                    _exit(EXIT_FAILURE);
                }
            }
            else
            {
                addProcess(PL, parsedInput, childp1);
                close(pipefd[1]);
                childp2 = fork();

                if (childp2 == -1)
                {
                    perror("Error forking child2");
                    exit(EXIT_FAILURE);
                }

                if (childp2 == 0)
                {
                    close(STDIN_FILENO);

                    if (dup(pipefd[0]) == -1)
                    {
                        perror("dup");
                        exit(EXIT_FAILURE);
                    }

                    close(pipefd[0]);

                    if (parsedInput->next->outputRedirect != NULL)
                    {
                        int fd = open(parsedInput->next->outputRedirect, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (fd == -1)
                        {
                            perror("Error opening file fuck");
                        }
                        if (dup2(fd, STDOUT_FILENO) == -1)
                        {
                            perror("Error redirecting stdout");
                        }
                        close(fd);
                    }

                    if (execvp(parsedInput->next->arguments[0], parsedInput->next->arguments) == -1)
                    {
                        perror("error in execv");
                        _exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    // parent process
                    addProcess(PL, parsedInput->next, childp2);

                    close(pipefd[0]);
                    waitpid(childp1, NULL, 0);
                    waitpid(childp2, NULL, 0);
                }
            }
        }
        else
        {
            execute(parsedInput, debug);
        }
    }

    return 0;
}