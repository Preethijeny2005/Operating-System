/*********************************************************************
Program : miniShell
--------------------------------------------------------------------
Simple skeleton code for a Linux/Unix/MINIX command line interpreter
--------------------------------------------------------------------
File : minishell.c
Compiler/System : gcc/linux
********************************************************************/

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>

#define NV 20    /* max number of command tokens */
#define NL 100   /* max number of characters per line */

void execute(char *argv[], int bg) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    } else if (pid == 0) {
        // Child process
        execvp(argv[0], argv);
        perror("execvp");
        exit(1);
    } else {
        // Parent process
        if (!bg) {
            int status;
            waitpid(pid, &status, 0);
        } else {
            printf("[#] %d\n", pid);
        }
    }
}

int main() {
    char line[NL];
    char *argv[NV];
    int bg;

    while (1) {
        // Show prompt only if interactive
        if (isatty(STDIN_FILENO)) {
            printf("myshell> ");
            fflush(stdout);
        }

        if (fgets(line, NL, stdin) == NULL) {
            // Ctrl-D or EOF
            break;
        }

        // Remove newline
        line[strcspn(line, "\n")] = '\0';

        if (strlen(line) == 0) {
            continue;
        }

        // Tokenize input
        int argc = 0;
        char *token = strtok(line, " ");
        while (token != NULL && argc < NV - 1) {
            argv[argc++] = token;
            token = strtok(NULL, " ");
        }
        argv[argc] = NULL;

        if (argc == 0) continue;

        // Check background
        bg = 0;
        if (strcmp(argv[argc - 1], "&") == 0) {
            bg = 1;
            argv[argc - 1] = NULL;
        }

        // Built-in: exit
        if (strcmp(argv[0], "exit") == 0) {
            break;
        }

        // Built-in: cd
        if (strcmp(argv[0], "cd") == 0) {
            if (argc < 2) {
                fprintf(stderr, "cd: missing argument\n");
            } else if (chdir(argv[1]) != 0) {
                perror("cd");
            }
            continue;
        }

        // Execute external command
        execute(argv, bg);

        // Reap background processes
        while (waitpid(-1, NULL, WNOHANG) > 0);
    }

    return 0;
}
