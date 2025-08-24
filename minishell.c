/*********************************************************************
Program : miniShell 
Version : 1.3
--------------------------------------------------------------------
Minimal command line interpreter with background jobs
--------------------------------------------------------------------
File : minishell.c
Compiler/System : gcc/linux, should also run on macOS
********************************************************************/

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#define NV 20   /* max number of command tokens */
#define NL 100  /* max input line length */

typedef struct job {
    int id;
    pid_t pid;
    char cmd[NL];
    struct job *next;
} Job;

Job *jobs = NULL;
int job_counter = 1;

/* --- Utility: Add job to list --- */
void add_job(pid_t pid, const char *cmd) {
    Job *j = malloc(sizeof(Job));
    j->id = job_counter++;
    j->pid = pid;
    strncpy(j->cmd, cmd, NL - 1);
    j->cmd[NL - 1] = '\0';
    j->next = jobs;
    jobs = j;
    printf("[%d] %d\n", j->id, j->pid);
    fflush(stdout);
}

/* --- Utility: Remove job once finished --- */
void remove_job(pid_t pid, int status) {
    Job **prev = &jobs;
    Job *curr = jobs;
    while (curr) {
        if (curr->pid == pid) {
            printf("[%d]+ Done                 %s\n", curr->id, curr->cmd);
            fflush(stdout);
            *prev = curr->next;
            free(curr);
            return;
        }
        prev = &curr->next;
        curr = curr->next;
    }
}

/* --- Reap background processes --- */
void sigchld_handler(int sig) {
    int saved_errno = errno;
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        remove_job(pid, status);
    }
    errno = saved_errno;
}

/* --- Parse input line into argv[] --- */
int parse_line(char *line, char *argv[]) {
    int argc = 0;
    char *token = strtok(line, " \t\n");
    while (token && argc < NV - 1) {
        argv[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    argv[argc] = NULL;
    return argc;
}

int main() {
    char line[NL];
    char *argv[NV];
    struct sigaction sa;

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    while (1) {
        printf(" msh> ");
        fflush(stdout);

        if (!fgets(line, NL, stdin))
            break;

        if (line[0] == '\n')
            continue;

        int argc = parse_line(line, argv);
        if (argc == 0)
            continue;

        int background = 0;
        if (strcmp(argv[argc - 1], "&") == 0) {
            background = 1;
            argv[argc - 1] = NULL;
        }

        if (strcmp(argv[0], "exit") == 0)
            break;

        pid_t pid = fork();
        if (pid == 0) {
            execvp(argv[0], argv);
            perror("execvp");
            exit(1);
        } else if (pid > 0) {
            if (background) {
                add_job(pid, argv[0]);
            } else {
                waitpid(pid, NULL, 0);
            }
        } else {
            perror("fork");
        }
    }
    return 0;
}
