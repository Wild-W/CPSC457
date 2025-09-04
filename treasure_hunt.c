#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>

#define MATRIX_ROW_COUNT 100
#define MATRIX_COL_COUNT 1000

int main() {
    srand(time(NULL)); // Seed rand()

    int matrix[MATRIX_ROW_COUNT][MATRIX_COL_COUNT]; // Initialize matrix
    int row = rand() % MATRIX_ROW_COUNT;
    int col = rand() % MATRIX_COL_COUNT; // Get a random x,y position
    matrix[row][col] = 1; // y = row, x = col

    printf("Matrix generated with treasure at row %d, column %d.\n", row, col);

    pid_t pid_row_map[MATRIX_ROW_COUNT];

    for (int i = 0; i < MATRIX_ROW_COUNT; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            for (int j = 0; j < MATRIX_COL_COUNT; ++j) {
                if (matrix[i][j] == 1) {
                    exit(EXIT_SUCCESS);
                }
            }
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("There was an error creating fork.");
            exit(EXIT_FAILURE);
        }
        // Parent process
        pid_row_map[pid] = i;
        printf("Child process of PID %d is searching row %d.\n", pid, i);
    }

    int remaining = MATRIX_ROW_COUNT;
    while (remaining != 0) {
        int status;
        pid_t pid = wait(&status);
        int exit_status = WEXITSTATUS(status);
        if (exit_status == EXIT_SUCCESS) {
            printf("Treasure found by process with PID %d on row %d\n", pid, pid_row_map[pid]);
        }
        --remaining;
    }

    puts("All processes have closed.");

    return 0;
}
