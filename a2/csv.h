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

int read_processes_csv(const char *filename, Process *out, int max_count);
int write_fcfs_details_csv(const char *filename, const ResultsDetails *rows, int count);
int write_fcfs_results_csv(const char *filename, const Results *rows, const int *scheduler_latencies, int count);

#endif /* CSV_H */
