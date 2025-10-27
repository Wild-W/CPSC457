#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_REFS 200000
#define MAX_PAGES 500
#define MAX_FRAMES 100
#define INF_NEXT 0x3fffffff

static void print_header_clk(const char *title, const char *varname);
static void print_row_clk(int var, long faults, long writes);
static void run_clk_once(int F, int nbits, int mshift, long *faults_out, long *writes_out);

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

static void print_header_clk(const char *title, const char *varname)
{
    printf("%s\n\n", title);
    puts("+----------+----------------+-----------------+");
    printf("| %8s | Page Faults    | Write-backs     |\n", varname);
    puts("+----------+----------------+-----------------+");
}

static void print_row_clk(int var, long faults, long writes)
{
    printf("| %8d | %14ld | %15ld |\n", var, faults, writes);
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

// Build position lists for OPT once
static void build_poslists(PosList pos[MAX_PAGES])
{
    static int counts[MAX_PAGES] = {0};
    for (int i = 0; i < MAX_PAGES; i++)
    {
        pos[i].pos = NULL;
        pos[i].count = 0;
        counts[i] = 0;
    }
    for (int i = 0; i < N; i++)
        counts[refs[i].page]++;
    for (int p = 0; p < MAX_PAGES; p++)
    {
        if (counts[p])
        {
            pos[p].pos = (int *)malloc(sizeof(int) * counts[p]);
            pos[p].count = counts[p];
            if (!pos[p].pos)
                die("OOM");
            counts[p] = 0; // reuse as fill index
        }
    }
    for (int i = 0; i < N; i++)
    {
        int p = refs[i].page;
        if (pos[p].pos)
            pos[p].pos[counts[p]++] = i;
    }
}

// OPT simulation for one frame count
static void run_opt_once(int F, const PosList pos[MAX_PAGES], long *faults_out, long *writes_out)
{
    Frame fr[MAX_FRAMES] = {0};
    int used = 0, next_arrival = 0;
    long faults = 0, writes = 0;

    // per-page pointer to "next occurrence >= current i"
    static int ptr[MAX_PAGES];
    for (int p = 0; p < MAX_PAGES; p++)
        ptr[p] = 0;

    for (int i = 0; i < N; i++)
    {
        int pg = refs[i].page, d = refs[i].dirty;

        // ensure ptr[pg] points at first occurrence >= i
        if (pos[pg].count)
        {
            while (ptr[pg] < pos[pg].count && pos[pg].pos[ptr[pg]] < i)
                ptr[pg]++;
        }

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
                fr[hit].dirty = 1;
        }
        else
        {
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
                // choose victim: farthest next use; tie -> smallest arrival (FIFO)
                int victim = -1;
                int best_next = -1; // larger is better (farther)
                for (int j = 0; j < F; j++)
                {
                    int p = fr[j].page;
                    int nx;
                    if (pos[p].count == 0)
                        nx = INF_NEXT;
                    else
                    {
                        int k = ptr[p];
                        // future strictly > i; if equal to i, use k+1
                        if (k < pos[p].count && pos[p].pos[k] <= i)
                            k++;
                        nx = (k < pos[p].count) ? pos[p].pos[k] : INF_NEXT;
                    }
                    if (nx > best_next)
                    {
                        best_next = nx;
                        victim = j;
                    }
                    else if (nx == best_next)
                    {
                        if (fr[j].arrival < fr[victim].arrival)
                            victim = j; // FIFO tie-break
                    }
                }
                if (fr[victim].dirty)
                    writes++;
                fr[victim].page = pg;
                fr[victim].dirty = d;
                fr[victim].arrival = next_arrival++;
                fr[victim].in_use = 1;
            }
        }

        // advance ptr for current page beyond i
        if (pos[pg].count)
        {
            if (ptr[pg] < pos[pg].count && pos[pg].pos[ptr[pg]] == i)
                ptr[pg]++;
        }
    }
    *faults_out = faults;
    *writes_out = writes;
}

// nbits: size of reference register; mshift: shift interval
static void run_clk_once(int F, int nbits, int mshift, long *faults_out, long *writes_out)
{
    Frame fr[MAX_FRAMES] = {0};
    unsigned int refreg[MAX_FRAMES] = {0};
    int used = 0, next_arrival = 0;
    long faults = 0, writes = 0;
    int since_shift = 0;
    unsigned int msb = (nbits >= 32) ? 0x80000000u : (1u << (nbits - 1));
    unsigned int mask = (nbits >= 32) ? 0xffffffffu : ((1u << nbits) - 1);

    for (int i = 0; i < N; i++)
    {
        int pg = refs[i].page, d = refs[i].dirty;
        int hit = -1;

        for (int j = 0; j < used; j++)
        {
            if (fr[j].in_use && fr[j].page == pg)
            {
                hit = j;
                break;
            }
        }

        if (hit != -1)
        {
            if (d)
                fr[hit].dirty = 1;
            refreg[hit] = (refreg[hit] | msb) & mask;
        }
        else
        {
            faults++;
            if (used < F)
            {
                fr[used].page = pg;
                fr[used].dirty = d;
                fr[used].arrival = next_arrival++;
                fr[used].in_use = 1;
                refreg[used] = msb; // set reference bit
                used++;
            }
            else
            {
                // victim: smallest refreg; tie -> smallest arrival (FIFO)
                int victim = 0;
                for (int j = 1; j < F; j++)
                {
                    if (refreg[j] < refreg[victim])
                        victim = j;
                    else if (refreg[j] == refreg[victim] && fr[j].arrival < fr[victim].arrival)
                        victim = j;
                }
                if (fr[victim].dirty)
                    writes++;
                fr[victim].page = pg;
                fr[victim].dirty = d;
                fr[victim].arrival = next_arrival++;
                fr[victim].in_use = 1;
                refreg[victim] = msb;
            }
        }

        // periodic shift after m references
        since_shift++;
        if (since_shift == mshift)
        {
            for (int j = 0; j < used; j++)
                refreg[j] >>= 1;
            since_shift = 0;
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
    else if (strcmp(argv[1], "OPT") == 0)
    {
        PosList pos[MAX_PAGES];
        build_poslists(pos);
        print_header("OPT");
        for (int f = 1; f <= 100; f++)
        {
            long faults, writes;
            run_opt_once(f, pos, &faults, &writes);
            print_row(f, faults, writes);
        }
        // free
        for (int p = 0; p < MAX_PAGES; p++)
            if (pos[p].pos)
                free(pos[p].pos);
    }
    else if (strcmp(argv[1], "CLK") == 0)
    {
        // Part II outputs (fixed frames = 50)
        const int F = 50;
        long faults, writes;

        // Table 1: m=10, vary n=1..32
        print_header_clk("CLK, m=10", "n"); // per spec
        for (int n = 1; n <= 32; n++)
        {
            run_clk_once(F, n, 10, &faults, &writes);
            print_row_clk(n, faults, writes);
        }

        printf("\n");

        // Table 2: n=8, vary m=1..100
        print_header_clk("CLK, n=8", "m"); // per spec
        for (int m = 1; m <= 100; m++)
        {
            run_clk_once(F, 8, m, &faults, &writes);
            print_row_clk(m, faults, writes);
        }
    }
    else
    {
        die("unknown algorithm (use FIFO or OPT)");
    }
    return 0;
}
