#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define CSV_IMPLEMENTATION
#include "csv.h"

#define MAX_RECORDS 1000
#define RR_LATENCY 20
#define Q1_Q 40
#define Q2_Q 80

static int cmp_arrival_then_pid(const void *a, const void *b) {
    const Process *pa = (const Process*)a;
    const Process *pb = (const Process*)b;
    if (pa->arrival_time != pb->arrival_time) return (pa->arrival_time < pb->arrival_time) ? -1 : 1;
    if (pa->pid != pb->pid) return (pa->pid < pb->pid) ? -1 : 1;
    return 0;
}

typedef struct {
    int *buf, cap, head, tail, count;
} Queue;

static void q_init(Queue *q, int cap) {
    q->buf = (int*)malloc(sizeof(int)* (cap > 0 ? cap : 1));
    q->cap = (cap > 0 ? cap : 1);
    q->head = q->tail = q->count = 0;
}
static inline int q_empty(Queue *q) { return q->count == 0; }
static inline void q_push(Queue *q, int v) { q->buf[q->tail] = v; q->tail = (q->tail+1)%q->cap; q->count++; }
static inline int q_pop(Queue *q) { int idx = q->head; q->head = (q->head+1)%q->cap; q->count--; return q->buf[idx]; }
static void q_free(Queue *q) { free(q->buf); q->buf=NULL; }

int main(void) {
    Process *procs = (Process*)malloc(sizeof(Process)*MAX_RECORDS);
    if (!procs) { puts("oom"); return 1; }

    int n = read_processes_csv(procs);
    if (n <= 0) { puts("no records"); free(procs); return 1; }

    qsort(procs, n, sizeof(Process), cmp_arrival_then_pid);

    int *rem = (int*)malloc(sizeof(int)*n);
    int *first = (int*)malloc(sizeof(int)*n);
    int *dispatches = (int*)malloc(sizeof(int)*n);
    int *finish = (int*)malloc(sizeof(int)*n);
    if (!rem || !first || !dispatches || !finish) { puts("oom"); return 1; }

    for (int i=0;i<n;i++){
        rem[i] = procs[i].burst_length;
        first[i] = -1;
        dispatches[i] = 0;
        finish[i] = -1;
    }

    Queue q1, q2, q3;
    q_init(&q1, n); q_init(&q2, n); q_init(&q3, n);

    int t = 0;
    int next = 0;
    if (n>0 && t < procs[0].arrival_time) t = procs[0].arrival_time;
    while (next < n && procs[next].arrival_time <= t) q_push(&q1, next++);

    int first_arrival = (n>0)? procs[0].arrival_time : 0;

    while (true) {
        if (q_empty(&q1) && q_empty(&q2) && q_empty(&q3)) {
            if (next >= n) break;
            t = procs[next].arrival_time;
            while (next < n && procs[next].arrival_time <= t) q_push(&q1, next++);
            continue;
        }

        int which = 0; // 0=q1,1=q2,2=q3
        if (!q_empty(&q1)) which = 0;
        else if (!q_empty(&q2)) which = 1;
        else which = 2;

        int i = (which==0) ? q_pop(&q1) : (which==1) ? q_pop(&q2) : q_pop(&q3);

        if (first[i] < 0) first[i] = t;
        t += RR_LATENCY;
        dispatches[i]++;

        int run = 0;
        if (which == 0) { run = (rem[i] < Q1_Q ? rem[i] : Q1_Q); }
        else if (which == 1) { run = (rem[i] < Q2_Q ? rem[i] : Q2_Q); }
        else { run = rem[i]; }

        t += run;
        rem[i] -= run;

        while (next < n && procs[next].arrival_time <= t) q_push(&q1, next++);

        if (rem[i] > 0) {
            if (which == 0) q_push(&q2, i);
            else if (which == 1) q_push(&q3, i);
            else q_push(&q3, i); // FCFS requeues shouldn't happen because we run to completion
        } else {
            finish[i] = t;
        }
    }

    long long sumW=0, sumTA=0, sumR=0;
    int last_finish = (n>0)? finish[0] : 0;
    for (int i=0;i<n;i++){
        if (finish[i] > last_finish) last_finish = finish[i];

        int turnaround = finish[i] - procs[i].arrival_time;
        int total_latency = dispatches[i] * RR_LATENCY;
        int waiting = turnaround - procs[i].burst_length - total_latency;
        int response = first[i] - procs[i].arrival_time;

        sumW += waiting;
        sumTA += turnaround;
        sumR += response;
    }

    double makespan = (last_finish - first_arrival) > 0 ? (double)(last_finish - first_arrival) : 1.0;
    double throughput = n>0 ? (double)n / makespan : 0.0;
    double avgW = n>0 ? (double)sumW / (double)n : 0.0;
    double avgTA = n>0 ? (double)sumTA / (double)n : 0.0;
    double avgR = n>0 ? (double)sumR / (double)n : 0.0;

    puts("Throughput,Avg_Waiting_Time,Avg_Turnaround_Time,Avg_Response_Time");
    printf("%.6f,%.2f,%.2f,%.2f\n", throughput, avgW, avgTA, avgR);

    q_free(&q1);
    q_free(&q2);
    q_free(&q3);
    free(rem);
    free(first);
    free(dispatches);
    free(finish);
    free(procs);
    return 0;
}