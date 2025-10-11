#ifndef CSV_H
#define CSV_H

#include <stddef.h>

typedef struct {
    int pid;
    int arrival_time;
    int time_until_first_response;
    int burst_length;
} Process;

typedef struct {
    int scheduler_latency;
    int pid;
    int arrival_time;
    int start_time;
    int finish_time;
    int turnaround_time;
    int waiting_time;
    int response_time;
} ResultsDetails;

typedef struct {
    double throughput;
    double avg_waiting_time;
    double avg_turnaround_time;
    double avg_response_time;
} Results;

int read_processes_csv(Process *out);
int write_fcfs_results_details_csv(const char *filename, const ResultsDetails *rows, int count);
int write_fcfs_results_csv(const char *filename, const Results *rows, const int *latencies, int count);
int write_rr_results_details_csv(const char *filename, const ResultsDetails *rows, int count);
int write_rr_results_csv(const char *filename, const Results *rows, const int *quantums, int count);

// need to do this because i can't tell gcc to link another file as per specification
#ifdef CSV_IMPLEMENTATION
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef RECORD_COUNT
#define RECORD_COUNT 1000
#endif
#ifndef MAX_LINE_LENGTH
#define MAX_LINE_LENGTH 256
#endif

/* stdin reader that skips an optional header */
static int is_digit(char c){ return c>='0' && c<='9'; }

int read_processes_csv(Process processes[]) {
    char line[MAX_LINE_LENGTH];
    int count = 0;

    if (!fgets(line, sizeof(line), stdin)) return 0;

    char *p = line;
    while (*p==' '||*p=='\t'||*p=='\r'||*p=='\n') p++;
    if ((unsigned char)p[0]==0xEF && (unsigned char)p[1]==0xBB && (unsigned char)p[2]==0xBF) p+=3;

    int header = *p && !is_digit(*p);

    if (!header) {
        char *save = NULL;
        char *tok = strtok_r(p, ",", &save); if (!tok) return 0;
        processes[count].pid = atoi(tok);
        tok = strtok_r(NULL, ",", &save); if (!tok) return 0;
        processes[count].arrival_time = atoi(tok);
        tok = strtok_r(NULL, ",", &save); if (!tok) return 0;
        processes[count].time_until_first_response = atoi(tok);
        tok = strtok_r(NULL, ",", &save); if (!tok) return 0;
        processes[count].burst_length = atoi(tok);
        count++;
    }

    while (count < RECORD_COUNT && fgets(line, sizeof(line), stdin)) {
        char *save = NULL;
        char *tok = strtok_r(line, ",", &save); if (!tok) continue;
        processes[count].pid = atoi(tok);
        tok = strtok_r(NULL, ",", &save); if (!tok) continue;
        processes[count].arrival_time = atoi(tok);
        tok = strtok_r(NULL, ",", &save); if (!tok) continue;
        processes[count].time_until_first_response = atoi(tok);
        tok = strtok_r(NULL, ",", &save); if (!tok) continue;
        processes[count].burst_length = atoi(tok);
        count++;
    }

    return count;
}

int write_fcfs_results_details_csv(const char *filename, const ResultsDetails *rows, int count) {
    if (!filename || !rows || count < 0) return -1;
    FILE *f = fopen(filename, "w"); if (!f) return -1;
    fprintf(f, "Scheduler_Latency,Pid,Arrival Time,Start Time,Finish Time,Turnaround Time,Waiting Time,Response Time\n");
    for (int i = 0; i < count; i++) {
        const ResultsDetails *r = &rows[i];
        fprintf(f, "%d,%d,%d,%d,%d,%d,%d,%d\n",
                r->scheduler_latency, r->pid, r->arrival_time, r->start_time,
                r->finish_time, r->turnaround_time, r->waiting_time, r->response_time);
    }
    fclose(f);
    return 0;
}

int write_fcfs_results_csv(const char *filename, const Results *rows, const int *latencies, int count) {
    if (!filename || !rows || !latencies || count < 0) return -1;
    FILE *f = fopen(filename, "w"); if (!f) return -1;
    fprintf(f, "Scheduler_Latency,Throughput,Avg_Waiting_Time,Avg_Turnaround_Time,Avg_Response_Time\n");
    for (int i = 0; i < count; i++) {
        fprintf(f, "%d,%.6f,%.2f,%.2f,%.2f\n",
                latencies[i], rows[i].throughput,
                rows[i].avg_waiting_time, rows[i].avg_turnaround_time, rows[i].avg_response_time);
    }
    fclose(f);
    return 0;
}

int write_rr_results_details_csv(const char *filename,
                                 const ResultsDetails *rows,
                                 int count) {
    if (!filename || !rows || count < 0) return -1;
    FILE *f = fopen(filename, "w");
    if (!f) return -1;
    fprintf(f, "Quantum_Size,Pid,Arrival Time,Start Time,Finish Time,Turnaround Time,Waiting Time,Response Time\n");
    for (int i = 0; i < count; i++) {
        const ResultsDetails *r = &rows[i];
        fprintf(f, "%d,%d,%d,%d,%d,%d,%d,%d\n",
                r->scheduler_latency, /* storing quantum here */
                r->pid, r->arrival_time, r->start_time, r->finish_time,
                r->turnaround_time, r->waiting_time, r->response_time);
    }
    fclose(f);
    return 0;
}

int write_rr_results_csv(const char *filename,
                         const Results *rows,
                         const int *quantums,
                         int count) {
    if (!filename || !rows || !quantums || count < 0) return -1;
    FILE *f = fopen(filename, "w");
    if (!f) return -1;
    fprintf(f, "Quantum_Size,Throughput,Avg_Waiting_Time,Avg_Turnaround_Time,Avg_Response_Time\n");
    for (int i = 0; i < count; i++) {
        fprintf(f, "%d,%.6f,%.2f,%.2f,%.2f\n",
                quantums[i], rows[i].throughput,
                rows[i].avg_waiting_time, rows[i].avg_turnaround_time, rows[i].avg_response_time);
    }
    fclose(f);
    return 0;
}

#endif /* CSV_IMPLEMENTATION */

#endif /* CSV_H */
