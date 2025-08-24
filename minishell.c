/*********************************************************************
 Program : miniShell (Task 2 Upgraded)
 Author  : Your Name
 Purpose : Minimal POSIX shell with background jobs, cd, robust errors
*********************************************************************/

#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#define NV  64   /* max number of command tokens */
#define NL  256  /* input buffer size */
#define MAX_JOBS 128

static char line[NL]; /* command input buffer */

/* ----------------------------- Jobs -------------------------------- */

typedef struct {
    int   id;                 /* job number starting at 1 */
    pid_t pid;                /* child pid */
    int   active;             /* 1 if running */
    char  cmdline[NL];        /* printable command */
} job_t;

static job_t jobs[MAX_JOBS];
static int job_count = 0;

/* find job by pid */
static int find_job_index(pid_t pid) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active && jobs[i].pid == pid) return i;
    }
    return -1;
}

/* add job, returns job index or -1 */
static int add_job(pid_t pid, const char *cmdline) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].active) {
            jobs[i].active = 1;
            jobs[i].pid = pid;
            jobs[i].id = ++job_count;
            snprintf(jobs[i].cmdline, sizeof(jobs[i].cmdline), "%s", cmdline);
            return i;
        }
    }
    return -1;
}

/* reap any finished background children and print completion lines */
static void reap_background_finished(void) {
    int status;
    while (1) {
        pid_t p = waitpid(-1, &status, WNOHANG); /* non-blocking */
        if (p == 0) break;        /* no more finished children */
        if (p == -1) {
            if (errno == ECHILD) break; /* no children at all */
            perror("waitpid");          /* system call error */
            break;
        }
        int idx = find_job_index(p);
        if (idx >= 0) {
            /* match the expected format: [#]+ Done                 <cmd> */
            printf("[%d]+ Done                 %s\n", jobs[idx].id, jobs[idx].cmdline);
            fflush(stdout);
            jobs[idx].active = 0;
        } else {
            /* a foreground child or unknown—ignore silently */
        }
    }
}

/* on EOF (non-interactive), wait for remaining background jobs to finish */
static void wait_for_all_background(void) {
    int any_active = 0;
    for (int i = 0; i < MAX_JOBS; i++) if (jobs[i].active) { any_active = 1; break; }
    if (!any_active) return;

    int status;
    pid_t p;
    /* block until all children are reaped */
    while ((p = waitpid(-1, &status, 0)) != -1) {
        int idx = find_job_index(p);
        if (idx >= 0) {
            printf("[%d]+ Done                 %s\n", jobs[idx].id, jobs[idx].cmdline);
            fflush(stdout);
            jobs[idx].active = 0;
        }
    }
    if (errno != ECHILD) perror("waitpid");
}

/* --------------------------- Utilities ------------------------------ */

static void prompt(void) {
    /* NOTE: remove prompts if your autograder requires silent mode */
    fprintf(stdout, "\n msh> ");
    fflush(stdout);
}

static void build_cmdline(char **v, int argc, char *out, size_t outsz) {
    out[0] = '\0';
    for (int i = 0; i < argc; i++) {
        size_t len = strlen(out);
        if (len + strlen(v[i]) + 2 < outsz) {
            if (i) strncat(out, " ", outsz - strlen(out) - 1);
            strncat(out, v[i], outsz - strlen(out) - 1);
        }
    }
}

/* ---------------------------- Builtins ------------------------------ */

static int builtin_cd(char **argv) {
    const char *target = argv[1];
    if (!target) {
        target = getenv("HOME");
        if (!target) {
            fprintf(stderr, "cd: HOME not set\n");
            return -1;
        }
    }
    if (chdir(target) == -1) {
        perror("chdir");
        return -1;
    }
    return 0;
}

/* ----------------------------- Main -------------------------------- */

int main(void) {
    char *v[NV];                /* token vector */
    const char *sep = " \t\n";  /* separators */

    /* Optional: ignore SIGINT in the parent so ^C hits children */
    struct sigaction sa = {0};
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGINT, &sa, NULL) == -1) perror("sigaction");

    while (1) {
        prompt();

        if (fgets(line, NL, stdin) == NULL) {
            /* EOF or error */
            if (ferror(stdin)) perror("fgets");
            /* before exiting, report & wait for any background jobs */
            wait_for_all_background();
            exit(0);
        }

        /* skip empty lines and comments */
        if (line[0] == '\n' || line[0] == '\0' || line[0] == '#') {
            reap_background_finished();
            continue;
        }

        /* tokenize */
        v[0] = strtok(line, sep);
        if (!v[0]) { reap_background_finished(); continue; }
        int argc = 1;
        for (; argc < NV; argc++) {
            v[argc] = strtok(NULL, sep);
            if (!v[argc]) break;
        }

        /* handle builtins (no fork) */
        if (strcmp(v[0], "cd") == 0) {
            builtin_cd(v);
            reap_background_finished();
            continue;
        }

        /* detect background '&' at end */
        int background = 0;
        if (argc > 0 && v[argc - 1] && strcmp(v[argc - 1], "&") == 0) {
            background = 1;
            v[argc - 1] = NULL; /* remove '&' */
            argc--;
        }

        /* build printable command line (for job messages) */
        char printable[NL];
        build_cmdline(v, argc, printable, sizeof(printable));

        /* fork */
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            reap_background_finished();
            continue;
        }

        if (pid == 0) {
            /* child: restore default SIGINT so ^C can terminate it */
            struct sigaction sc = {0};
            sc.sa_handler = SIG_DFL;
            if (sigaction(SIGINT, &sc, NULL) == -1) perror("sigaction");

            /* exec */
            execvp(v[0], v);
            /* if we get here, exec failed */
            perror("execvp");
            _exit(127); /* ensure child terminates */
        }

        /* parent */
        if (background) {
            int idx = add_job(pid, printable);
            if (idx == -1) {
                fprintf(stderr, "jobs table full; running in foreground\n");
                int status;
                if (waitpid(pid, &status, 0) == -1) perror("waitpid");
            } else {
                /* expected start line: [#] PID */
                printf("[%d] %d\n", jobs[idx].id, jobs[idx].pid);
                fflush(stdout);
            }
        } else {
            /* foreground: wait until it finishes */
            int status;
            if (waitpid(pid, &status, 0) == -1) perror("waitpid");
        }

        /* after running a command, reap any background jobs that finished meanwhile */
        reap_background_finished();
    }
}