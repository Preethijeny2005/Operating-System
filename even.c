// Purpose:Prints the first n even numbers, sleeping 5s between each.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>


// helped function to handle the signals 
void handle_signal(int sig) {
    if (sig == SIGHUP) {
        printf("Ouch!\n");
        fflush(stdout);  
    } else if (sig == SIGINT) {
        printf("Yeah!\n");
        fflush(stdout);
    }
} // Handle_signal function 

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Does not accept negative input numbers
    // Not sure wheather gradescope will test for this 

    int number = atoi(argv[1]);
    if (number < 0) {
        fprintf(stderr, "Error: The number must not be negative\n");
        exit(EXIT_FAILURE);
    }

    // Chekching for signal errors and determining the output
    if (signal(SIGHUP, handle_signal) == SIG_ERR) {
        perror("signal SIGHUP");
        exit(EXIT_FAILURE);
    }
    if (signal(SIGINT, handle_signal) == SIG_ERR) {
        perror("signal SIGINT");
        exit(EXIT_FAILURE);
    }

    // Printing the even numbers, with a 5 second pause between printing 
    // each number
    for (int i = 0; i < number; i++) {
        printf("%d\n", i * 2);
        fflush(stdout);  
        sleep(5);        
    }

    return 0;
} // main function
