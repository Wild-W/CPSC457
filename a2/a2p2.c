#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define CSV_IMPLEMENTATION
#include "csv.h"

#define RR_LATENCY 20
#define MAX_RECORDS 1000
#define MIN_Q 1
#define MAX_Q 200

static int cmp_arrival_then_pid(const void *a, const void *b) {
    const Process *pa = (const Process*)a;
    const Process *pb = (const Process*)b;
    if (pa->arrival_time != pb->arrival_time) return (pa->arrival_time < pb->arrival_time) ? -1 : 1;
    if (pa->pid != pb->pid) return (pa->pid < pb->pid) ? -1 : 1;
    return 0;
}

static void simulate_rr_one(const Process *procs_sorted, int n, int q,
                            ResultsDetails *out_details, Results *out_agg)
{
    int *rem = (int*)malloc(sizeof(int)*n);
    int *first = (int*)malloc(sizeof(int)*n);
    int *dispatches = (int*)malloc(sizeof(int)*n);
    int *finish = (int*)malloc(sizeof(int)*n);
    int *arrival = (int*)malloc(sizeof(int)*n);
    int *pid = (int*)malloc(sizeof(int)*n);

    for (int i=0;i<n;i++){
        rem[i] = procs_sorted[i].burst_length;
        first[i] = -1;
        dispatches[i] = 0;
        finish[i] = -1;
        arrival[i] = procs_sorted[i].arrival_time;
        pid[i] = procs_sorted[i].pid;
    }

    // circular queue: capacity n
    int cap = (n > 0) ? n : 1;
    int *qbuf = (int*)malloc(sizeof(int)*cap);
    int head=0, tail=0, qcount=0;
    #define ENQ(v) do { qbuf[tail] = (v); tail = (tail+1)%cap; qcount++; } while(0)
    #define DEQ() (qcount--, head = (head+1)%cap, qbuf[(head-1+cap)%cap])
    #define QEMPTY() (qcount==0)

    int t = 0;
    int next = 0;
    if (n>0 && t < procs_sorted[0].arrival_time) t = procs_sorted[0].arrival_time;
    while (next < n && procs_sorted[next].arrival_time <= t) ENQ(next++);

    int first_arrival = (n>0)? procs_sorted[0].arrival_time : 0;
    while (true) {
        if (QEMPTY()) {
            if (next >= n) break;
            t = procs_sorted[next].arrival_time;
            while (next < n && procs_sorted[next].arrival_time <= t) ENQ(next++);
            continue;
        }

        int i = DEQ();

        t += RR_LATENCY;
        dispatches[i]++;

        if (first[i] < 0) first[i] = t;

        int run = rem[i] < q ? rem[i] : q;
        t += run;
        rem[i] -= run;

        while (next < n && procs_sorted[next].arrival_time <= t) ENQ(next++);

        if (rem[i] > 0) {
            ENQ(i);
        } else {
            finish[i] = t;
        }
    }

    long long sumW=0, sumTA=0, sumR=0;
    int last_finish = (n>0)? finish[0] : 0;
    for (int i=0;i<n;i++){
        int st = (first[i]>=0)? first[i] : t;
        int ft = finish[i];
        if (ft > last_finish) last_finish = ft;

        ResultsDetails *d = &out_details[i];
        d->scheduler_latency = q; // store quantum
        d->pid = pid[i];
        d->arrival_time = arrival[i];
        d->start_time = st;
        d->finish_time = ft;
        d->turnaround_time = ft - arrival[i];

        int total_latency = dispatches[i] * RR_LATENCY;
        d->waiting_time = d->turnaround_time - procs_sorted[i].burst_length - total_latency;

        d->response_time = (st - arrival[i]) + procs_sorted[i].time_until_first_response;

        sumW += d->waiting_time;
        sumTA += d->turnaround_time;
        sumR += d->response_time;
    }

    double makespan = (last_finish - first_arrival) > 0 ? (double)(last_finish - first_arrival) : 1.0;

    out_agg->throughput = n>0 ? (double)n / makespan : 0.0;
    out_agg->avg_waiting_time = n>0 ? (double)sumW / (double)n : 0.0;
    out_agg->avg_turnaround_time = n>0 ? (double)sumTA / (double)n : 0.0;
    out_agg->avg_response_time = n>0 ? (double)sumR / (double)n : 0.0;

    free(rem);
    free(first);
    free(dispatches);
    free(finish);
    free(arrival);
    free(pid);
    free(qbuf);
}

int main(void) {
    Process *procs = (Process*)malloc(sizeof(Process)*MAX_RECORDS);
    if (!procs) { puts("oom"); return 1; }

    int n = read_processes_csv(procs);
    if (n <= 0) { puts("no records"); free(procs); return 1; }

    qsort(procs, n, sizeof(Process), cmp_arrival_then_pid);

    const int QN = MAX_Q - MIN_Q + 1;
    ResultsDetails *all_details = (ResultsDetails*)malloc(sizeof(ResultsDetails)*n*QN);
    Results *agg = (Results*)malloc(sizeof(Results)*QN);
    int *quantums = (int*)malloc(sizeof(int)*QN);
    if (!all_details || !agg || !quantums) {
        puts("oom");
        free(procs); free(all_details); free(agg); free(quantums);
        return 1;
    }

    for (int q = MIN_Q, idx = 0; q <= MAX_Q; q++, idx++) {
        quantums[idx] = q;
        ResultsDetails *block = all_details + idx * n;
        simulate_rr_one(procs, n, q, block, &agg[idx]);
    }

    if (write_rr_results_details_csv("rr_results_details.csv", all_details, n*QN) != 0)
        puts("write rr details failed");
    if (write_rr_results_csv("rr_results.csv", agg, quantums, QN) != 0)
        puts("write rr results failed");

    puts("RR simulation completed! Results saved to rr_results.csv\nAverage results saved to rr_results_details.csv");

    free(procs);
    free(all_details);
    free(agg);
    free(quantums);
    return 0;
}
