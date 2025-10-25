#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_REFS 200000
#define MAX_PAGES 500
#define MAX_FRAMES 100
#define INF_NEXT 0x3fffffff

typedef struct
{
    int page, dirty;
} Ref;
typedef struct
{
    int *pos, count;
} PosList;
typedef struct
{
    int page, dirty, arrival, in_use;
} Frame;

static Ref refs[MAX_REFS];
static int N = 0;

static void die(const char *m)
{
    fprintf(stderr, "%s\n", m);
    exit(1);
}

static void load_input(void)
{
    char line[128];

    // skip first line unconditionally (e.g., header)
    if (!fgets(line, sizeof(line), stdin))
        die("No input read.");

    while (fgets(line, sizeof(line), stdin))
    {
        char *p = line;
        while (*p == ' ' || *p == '\t')
            ++p;

        // find comma
        char *q = p;
        while (*q && *q != ',')
            ++q;
        if (!*q)
            continue; // no comma, skip
        *q++ = '\0';

        // parse numbers
        int page = atoi(p);
        while (*q == ' ' || *q == '\t')
            ++q;
        int dirty = atoi(q) ? 1 : 0;

        if (page < 0 || page >= MAX_PAGES)
            continue;

        refs[N].page = page;
        refs[N].dirty = dirty;
        ++N;
        if (N >= MAX_REFS)
            break;
    }
    if (N == 0)
        die("No input read.");
}

// prints the required table header
static void print_header(const char *alg)
{
    printf("%s\n", alg);
    puts("+----------+----------------+-----------------+");
    puts("| Frames   | Page Faults    | Write-backs     |");
    puts("+----------+----------------+-----------------+");
}

static void print_row(int frames, long faults, long writes)
{
    // right-align inside fixed columns
    printf("| %8d | %14ld | %15ld |\n", frames, faults, writes);
    puts("+----------+----------------+-----------------+");
}

// FIFO simulation for one frame count
static void run_fifo_once(int F, long *faults_out, long *writes_out)
{
    Frame fr[MAX_FRAMES] = {0};
    int used = 0, next_arrival = 0;
    long faults = 0, writes = 0;

    for (int i = 0; i < N; i++)
    {
        int pg = refs[i].page, d = refs[i].dirty;
        int hit = -1;
        for (int j = 0; j < used; j++)
            if (fr[j].in_use && fr[j].page == pg)
            {
                hit = j;
                break;
            }

        if (hit != -1)
        {
            if (d)
                fr[hit].dirty = 1; // accumulate dirty while resident
            continue;
        }

        // miss
        faults++;
        if (used < F)
        {
            fr[used].page = pg;
            fr[used].dirty = d;
            fr[used].arrival = next_arrival++;
            fr[used].in_use = 1;
            used++;
        }
        else
        {
            // evict FIFO: smallest arrival
            int victim = 0;
            for (int j = 1; j < F; j++)
                if (fr[j].arrival < fr[victim].arrival)
                    victim = j;
            if (fr[victim].dirty)
                writes++;
            fr[victim].page = pg;
            fr[victim].dirty = d;
            fr[victim].arrival = next_arrival++;
            fr[victim].in_use = 1;
        }
    }
    *faults_out = faults;
    *writes_out = writes;
}

int main(int argc, char **argv)
{
    if (argc != 2)
        die("usage: ./a3 FIFO|OPT < input.csv");
    load_input();

    if (strcmp(argv[1], "FIFO") == 0)
    {
        print_header("FIFO");
        for (int f = 1; f <= 100; f++)
        {
            long faults, writes;
            run_fifo_once(f, &faults, &writes);
            print_row(f, faults, writes);
        }
    }
    else
    {
        die("unknown algorithm (use FIFO or OPT)");
    }
    return 0;
}
