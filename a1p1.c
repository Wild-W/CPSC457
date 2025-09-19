#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>

#define MATRIX_ROW_COUNT 100
#define MATRIX_COL_COUNT 1000

int find(pid_t *array, int len, pid_t val) {
    for (int i = 0; i < len; ++i) {
        if (array[i] == val) {
            return i;
        }
    }
    return -1; // didn't find anything
}

int main(int argc, char **argv) {
    int matrix[MATRIX_ROW_COUNT][MATRIX_COL_COUNT]; // Initialize matrix
    memset(matrix, 0, sizeof(matrix));

    int row = 0;
    int col = 0;

    int c;
    bool found = false;
    while ((c = fgetc(stdin)) != EOF) {
        switch (c) {
            case '0':
            ++col;
            break;

            case '\n': // input files from D2L end with LF so this should suffice
            ++row;
            col = 0;
            break;

            case ' ':
            break;

            case '1': // found the treasure location
            found = true;
        }
        if (found) {
            matrix[row][col] = 1; // y = row, x = col
            break;
        }
    }

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
            perror("There was an error creating fork");
            return -1;
        }
        // Parent process
        pid_row_map[i] = pid;
    }

    int remaining = MATRIX_ROW_COUNT;
    bool success;
    while (remaining != 0) {
        int status;
        pid_t pid = wait(&status);
        int exit_status = WEXITSTATUS(status);
        if (exit_status == EXIT_SUCCESS) {
            int found_row = find(pid_row_map, sizeof(pid_row_map) / sizeof(int), pid);
            int found_column = -1;
            for (int i = 0; i < MATRIX_COL_COUNT; ++i) {
                if (matrix[found_row][i] == 1) {
                    found_column = i;
                }
            }
            if (found_column == -1) {
                perror("Something went wrong, found_column should not be -1! Aborting.");
                return 1;
            }
            printf("The treasure was found by child with PID %d at row %d, column %d\n", pid, found_row, found_column);
            success = true;
        }
        --remaining;
    }

    if (!success) printf("No treasure was found in the matrix\n");

    return 0;
}
