/* a2p1.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csv.h"

#ifndef MIN_LATENCY
#define MIN_LATENCY 1
#endif
#ifndef MAX_LATENCY
#define MAX_LATENCY 200
#endif
#ifndef MAX_RECORDS
#define MAX_RECORDS 1000
#endif

static int cmp_arrival_then_pid(const void *a, const void *b) {
    const Process *pa = (const Process*)a;
    const Process *pb = (const Process*)b;
    if (pa->arrival_time != pb->arrival_time) return (pa->arrival_time < pb->arrival_time) ? -1 : 1;
    if (pa->pid != pb->pid) return (pa->pid < pb->pid) ? -1 : 1;
    return 0;
}

static void simulate_fcfs_one(const Process *procs_sorted, int n, int L,
                              ResultsDetails *out_details, Results *out_agg)
{
    long long sum_w = 0, sum_ta = 0, sum_r = 0;
    int t = 0;
    int first_arrival = n > 0 ? procs_sorted[0].arrival_time : 0;

    for (int i = 0; i < n; i++) {
        const Process *p = &procs_sorted[i];
        if (t < p->arrival_time) t = p->arrival_time;
        int start = t + L;
        int finish = start + p->burst_length;

        ResultsDetails *d = &out_details[i];
        d->scheduler_latency = L;
        d->pid = p->pid;
        d->arrival_time = p->arrival_time;
        d->start_time = start;
        d->finish_time = finish;
        d->turnaround_time = finish - p->arrival_time;
        d->waiting_time = start - p->arrival_time;
        d->response_time = d->waiting_time + p->time_until_first_response;

        sum_w  += d->waiting_time;
        sum_ta += d->turnaround_time;
        sum_r  += d->response_time;

        t = finish;
    }

    int last_finish = n > 0 ? out_details[n-1].finish_time : 0;
    double span = (last_finish - first_arrival) > 0 ? (double)(last_finish - first_arrival) : 1.0;

    out_agg->throughput = n > 0 ? (double)n / span : 0.0;
    out_agg->avg_waiting_time = n > 0 ? (double)sum_w / (double)n : 0.0;
    out_agg->avg_turnaround_time = n > 0 ? (double)sum_ta / (double)n : 0.0;
    out_agg->avg_response_time = n > 0 ? (double)sum_r / (double)n : 0.0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: %s <input_csv>\n", argv[0]);
        return 1;
    }
    const char *input_csv = argv[1];

    Process *processes = (Process*)malloc(sizeof(Process) * MAX_RECORDS);
    if (!processes) { puts("oom"); return 1; }

    int n = read_processes_csv(input_csv, processes);
    if (n <= 0) {
        puts("no records");
        free(processes);
        return 1;
    }

    qsort(processes, n, sizeof(Process), cmp_arrival_then_pid);

    int latency_count = (MAX_LATENCY >= MIN_LATENCY) ? (MAX_LATENCY - MIN_LATENCY + 1) : 0;
    if (latency_count <= 0) {
        puts("bad latency range");
        free(processes);
        return 1;
    }

    ResultsDetails *all_details = (ResultsDetails*)malloc(sizeof(ResultsDetails) * n * latency_count);
    Results *agg = (Results*)malloc(sizeof(Results) * latency_count);
    int *latencies = (int*)malloc(sizeof(int) * latency_count);
    if (!all_details || !agg || !latencies) {
        puts("oom");
        free(processes); free(all_details); free(agg); free(latencies);
        return 1;
    }

    for (int L = MIN_LATENCY, idx = 0; L <= MAX_LATENCY; L++, idx++) {
        latencies[idx] = L;
        ResultsDetails *block = all_details + idx * n;
        simulate_fcfs_one(processes, n, L, block, &agg[idx]);
    }

    if (write_fcfs_results_details_csv("fcfs_results_details.csv", all_details, n * latency_count) != 0)
        puts("write details failed");
    if (write_fcfs_results_csv("fcfs_results.csv", agg, latencies, latency_count) != 0)
        puts("write results failed");

    puts("Simulation completed! Process table results saved to fcfs_results_details.csv\nAverage results saved to fcfs_results.csv");

    free(processes);
    free(all_details);
    free(agg);
    free(latencies);
    return 0;
}
