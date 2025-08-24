/*********************************************************************
Program : miniShell Version : 1.3
--------------------------------------------------------------------
Simple skeleton code for Linux/Unix command line interpreter
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


#define NV 20     /* max number of command tokens */
#define NLIN 1024 /* max command line size */

struct job {
    pid_t pid;
    char cmd[NLIN];
    struct job *next;
};

struct job *jobs = NULL;

/* remove finished job */
void remove_job(pid_t pid) {
    struct job **cur = &jobs;
    while (*cur) {
        if ((*cur)->pid == pid) {
            struct job *tmp = *cur;
            *cur = (*cur)->next;
            free(tmp);
            return;
        }
        cur = &((*cur)->next);
    }
}

/* reap children when they exit */
void sigchld_handler(int sig) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        struct job *cur = jobs;
        while (cur) {
            if (cur->pid == pid) {
                printf("[#]+ Done                 %s\n", cur->cmd);
                fflush(stdout);
                remove_job(pid);
                break;
            }
            cur = cur->next;
        }
    }
}

int main() {
    char line[NLIN];
    char *argv[NV];
    char *p;
    int background;

    signal(SIGCHLD, sigchld_handler);

    while (1) {
        /* prompt */
        printf("miniShell> ");
        fflush(stdout);

        if (!fgets(line, NLIN, stdin)) {
            break; /* EOF */
        }

        /* strip newline */
        line[strcspn(line, "\n")] = 0;

        /* check empty input */
        if (strlen(line) == 0) continue;

        /* tokenize */
        int argc = 0;
        background = 0;
        p = strtok(line, " ");
        while (p != NULL && argc < NV-1) {
            if (strcmp(p, "&") == 0) {
                background = 1;
            } else {
                argv[argc++] = p;
            }
            p = strtok(NULL, " ");
        }
        argv[argc] = NULL;

        if (argc == 0) continue;

        /* builtin exit */
        if (strcmp(argv[0], "exit") == 0) {
            break;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            /* child */
            execvp(argv[0], argv);
            perror("execvp");
            exit(1);
        } else {
            /* parent */
            if (background) {
                /* add to jobs list */
                struct job *newjob = malloc(sizeof(struct job));
                newjob->pid = pid;
                snprintf(newjob->cmd, sizeof(newjob->cmd), "%s", argv[0]);
                newjob->next = jobs;
                jobs = newjob;

                printf("[#] %d\n", pid);
                fflush(stdout);
            } else {
                waitpid(pid, NULL, 0);
            }
        }
    }

    return 0;
}
