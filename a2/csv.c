#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RECORD_COUNT 1000
#define MAX_LINE_LENGTH 256
#define MIN_LATENCY 1
#define MAX_LATENCY 200

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

int read_csv(const char *filename, Process processes[]) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file '%s'\n", filename);
        return -1;
    }
    
    char line[MAX_LINE_LENGTH];
    int count = 0;
    
    // Skip header line
    if (fgets(line, MAX_LINE_LENGTH, file) == NULL) {
        printf("Error: Empty file\n");
        fclose(file);
        return -1;
    }
    
    // Read process data
    while (fgets(line, MAX_LINE_LENGTH, file) && count < RECORD_COUNT) {
        char *token = strtok(line, ",");
        if (token == NULL) continue;
        processes[count].pid = atoi(token);
        
        token = strtok(NULL, ",");
        if (token == NULL) continue;
        processes[count].arrival_time = atoi(token);
        
        token = strtok(NULL, ",");
        if (token == NULL) continue;
        processes[count].time_until_first_response = atoi(token);
        
        token = strtok(NULL, ",");
        if (token == NULL) continue;
        processes[count].burst_length = atoi(token);
        
        count++;
    }
    
    fclose(file);
    return count;
}
