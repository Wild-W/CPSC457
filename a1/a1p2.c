#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <math.h>

int is_prime(int num)
{
    if (num < 2)
        return 0;
    for (int i = 2; i <= sqrt(num); i++)
    {
        if (num % i == 0)
            return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s LOWER_BOUND UPPER_BOUND N\n", argv[0]);
        return 1;
    }

    long lower = atol(argv[1]);
    long upper = atol(argv[2]);
    long N_in = atol(argv[3]);

    if (lower > upper || N_in <= 0)
    {
        fprintf(stderr, "Invalid arguments.\n");
        return 1;
    }

    long total = upper - lower + 1;
    long N_final = (N_in > total) ? total : N_in; // final number of children
    long max_block = (total / N_final) + (total % N_final);

    // [counts[N_final]] [data[N_final * max_block]]
    size_t ints_needed = (size_t)N_final + (size_t)N_final * (size_t)max_block;
    size_t shm_bytes = ints_needed * sizeof(int);

    int shmid = shmget(IPC_PRIVATE, shm_bytes, IPC_CREAT | 0666);
    if (shmid == -1)
        return 1;

    int *shm = (int *)shmat(shmid, NULL, 0);
    if (shm == (void *)-1)
        return 1;

    int *counts = shm;
    int *data = shm + N_final;

    for (long i = 0; i < N_final; ++i)
        counts[i] = 0;

    for (long i = 0; i < N_final; ++i)
    {
        long start = lower + i * (total / N_final);
        long end = (i == N_final - 1) ? upper : (start + (total / N_final) - 1);

        pid_t pid = fork();
        if (pid == 0)
        {
            // Child: find primes in [start, end], write to its own block
            int *this_block = data + (i * (long)max_block);
            int cnt = 0;
            for (long n = start; n <= end; ++n)
                if (is_prime((int)n))
                    this_block[cnt++] = (int)n;
            counts[i] = cnt;
            exit(0);
        }
        else
        {
            printf("Child PID %d checking range [%ld, %ld]\n", (int)pid, start, end);
        }
    }

    // Wait for all children
    for (long i = 0; i < N_final; ++i)
        wait(NULL);

    puts("\nParent: All children finished. Primes found:");
    for (long i = 0; i < N_final; ++i)
    {
        int cnt = counts[i];
        int *this_block = data + (i * (long)max_block);
        for (int k = 0; k < cnt; ++k)
            printf("%d ", this_block[k]);
    }
    puts("");

    // Clean up
    shmdt(shm);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
