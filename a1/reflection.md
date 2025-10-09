# Assignment 1 Reflection

Github repository link: https://github.com/Wild-W/CPSC457

## Part 1

### What were the most challenging aspects to you of this program and how did you tackle them?

Something that generated a lot of confusion for me was how I was supposed to find the row and column of the treasure after a child successfully identified it without shared memory. It felt unintuitive to have to find these numbers again in the parent process because that felt like it was undermining the whole point of creating multiple processes, that is, to decrease execution time. My initial idea was to encode the row and column number in the exit status code of the child process, but the assignment specified that the child process that finds the treasure should send a success code to the parent, not to mention the fact that a single byte can't hold the column number as it is too large. The solution I came to in the end was to store a 100 element array of pid_t values, where the index of the array is the row number, and the value was the process id of the child searching that row, then upon receiving word back from the child, perform a linear search on the array for that child's row, then iterate over all columns of the row until the parent found the treasure. This seems to be at least partly the intended solution, and may theoretically be faster (disregarding time to open a process) as instead of iterating an upper bound of $100 \times 1000$ times in the parent process, we iterate an upper bound of $100 + 1000$ times instead.

## Part 2

### How did I prevent the program from creating more than the required number of processes?

This line of code:
```c
long N_final = (N_in > total) ? total : N_in; // final number of children
```
is what determines exactly how many children should be created. It does this by capping the number of children by the number of values in the range.

### How did I divide work among child processes?

I split the inputted range into nearly equal length N_final contiguous chunks. Each child `i` gets `start = lower + i*(total / N_final)` and `end = (i == N_final-1) ? upper : start + (total / N_final) - 1`, so the first `N_final-1` children handle exactly `(total / N_final)` numbers and the last child absorbs the remainder.

### How did I make access to shared memory by child processes safe?

Children were given non-overlapping blocks in the shared array as per the assignment's suggestion. Since the parent does not read from the array until all processes are finished, there is also no situation where the parent may read data that is currently being written to. Avoiding concurrent read/write situations means that we don't need to use locks to access critical sections.
