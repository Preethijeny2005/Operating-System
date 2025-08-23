/*********************************************************************
 * Purpose : Prints the first n even numbers, sleeping 5s between each.
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void handle_signal(int sig) {
    if (sig == SIGHUP) {
        printf("Ouch!\n");
        fflush(stdout);  
    } else if (sig == SIGINT) {
        printf("Yeah!\n");
        fflush(stdout);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n = atoi(argv[1]);
    if (n < 0) {
        fprintf(stderr, "Error: The number must be non-negative\n");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGHUP, handle_signal) == SIG_ERR) {
        perror("signal SIGHUP");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGINT, handle_signal) == SIG_ERR) {
        perror("signal SIGINT");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n; i++) {
        printf("%d\n", i * 2);
        fflush(stdout);  
        sleep(5);        
    }

    return 0;
}
