#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "process.h"
#include "queue.h"
#include "scheduler.h"

int num_algorithms()
{
    return sizeof(algorithmsNames) / sizeof(char *);
}

int num_modalities()
{
    return sizeof(modalitiesNames) / sizeof(char *);
}

size_t initFromCSVFile(char *filename, Process **procTable)
{
    FILE *f = fopen(filename, "r");

    size_t procTableSize = 10;

    *procTable = malloc(procTableSize * sizeof(Process));
    Process *_procTable = *procTable;

    if (f == NULL)
    {
        perror("initFromCSVFile():::Error Opening File:::");
        exit(1);
    }

    char *line = NULL;
    size_t buffer_size = 0;
    size_t nprocs = 0;
    while (getline(&line, &buffer_size, f) != -1)
    {
        if (line != NULL)
        {
            Process p = initProcessFromTokens(line, ";");

            if (nprocs == procTableSize - 1)
            {
                procTableSize = procTableSize + procTableSize;
                _procTable = realloc(_procTable, procTableSize * sizeof(Process));
            }

            _procTable[nprocs] = p;

            nprocs++;
        }
    }
    free(line);
    fclose(f);
    return nprocs;
}

size_t getTotalCPU(Process *procTable, size_t nprocs)
{
    size_t total = 0;
    for (int p = 0; p < nprocs; p++)
    {
        total += (size_t)procTable[p].burst;
    }
    return total;
}

int getCurrentBurst(Process *proc, int current_time)
{
    int burst = 0;
    for (int t = 0; t < current_time; t++)
    {
        if (proc->lifecycle[t] == Running)
        {
            burst++;
        }
    }
    return burst;
}

int run_dispatcher(Process *procTable, size_t nprocs, int algorithm, int modality, int quantum) {

    int current_time = 0;
    Process *current_proc = NULL;
    size_t completed_procs = 0;

    qsort(procTable, nprocs, sizeof(Process), compareArrival);
    init_queue();
    
    size_t duration = getTotalCPU(procTable, nprocs) + 1;

    for (int p = 0; p < nprocs; p++) {
        procTable[p].lifecycle = malloc(duration * sizeof(int));
        for (int t = 0; t < duration; t++) {
            procTable[p].lifecycle[t] = -1;
        }
        procTable[p].waiting_time = 0;
        procTable[p].return_time = 0;
        procTable[p].response_time = -1; 
        procTable[p].completed = false;
    }

    int next_arrival_index = 0;
    int quantum_counter = 0;

    while (completed_procs < nprocs) {

        while (next_arrival_index < nprocs && procTable[next_arrival_index].arrive_time <= current_time) {
            Process *p = &procTable[next_arrival_index];
            enqueue(p);
            next_arrival_index++;
        }
   
        if (algorithm == SJF || algorithm == PRIORITIES) {
            Process *ready_list = transformQueueToList();
            size_t queue_size = get_queue_size();
            
            if (ready_list != NULL) {
                int (*comparator)(const void *, const void *) = NULL;
                
                if (algorithm == SJF) {
                    comparator = compareBurst; 
                } else if (algorithm == PRIORITIES) {
                    comparator = comparePriority;
                }
                
                if (comparator != NULL) {
                    qsort(ready_list, queue_size, sizeof(Process), comparator);
                    setQueueFromList(ready_list);
                }
                free(ready_list);
            }
        }

        Process *next_proc_in_queue = NULL;
        if (get_queue_size() > 0) {
            next_proc_in_queue = dequeue();
        }

        if (current_proc == NULL && next_proc_in_queue != NULL) {
            current_proc = next_proc_in_queue;
            quantum_counter = 0;
        } else if (current_proc != NULL && next_proc_in_queue != NULL) {
            bool should_preempt = false;

            if (modality == PREEMPTIVE) {
                if (algorithm == SJF) { // SJRT
                    int current_remaining = current_proc->burst - getCurrentBurst(current_proc, current_time);
                    int next_remaining = next_proc_in_queue->burst - getCurrentBurst(next_proc_in_queue, current_time);
                    
                    if (next_remaining < current_remaining) {
                        should_preempt = true;
                    }
                } else if (algorithm == PRIORITIES) {
                    if (next_proc_in_queue->priority < current_proc->priority) {
                        should_preempt = true;
                    }
                }
            }
            
            if (should_preempt) {
                enqueue(current_proc);
                current_proc = next_proc_in_queue;
                quantum_counter = 0;
            } else {
                enqueue(next_proc_in_queue); 
            }
        } else if (current_proc == NULL && get_queue_size() == 0 && next_arrival_index < nprocs) {
            current_time = procTable[next_arrival_index].arrive_time;
            continue;
        }
        
        if (completed_procs == nprocs) {
            break; 
        }

        if (current_proc != NULL) {
            current_proc->lifecycle[current_time] = Running;

            if (current_proc->response_time == -1) {
                current_proc->response_time = current_time - current_proc->arrive_time;
            }

            Process *waiting_list = transformQueueToList();
            if (waiting_list != NULL) {
                size_t queue_size = get_queue_size();
                for (int i = 0; i < queue_size; i++) {
                    waiting_list[i].waiting_time++;
                }
                setQueueFromList(waiting_list); 
                free(waiting_list);
            }
  
            int executed_burst = getCurrentBurst(current_proc, current_time + 1);
            quantum_counter++;

            if (executed_burst >= current_proc->burst) {
                current_proc->lifecycle[current_time] = Finished;
                current_proc->completed = true;
                current_proc->return_time = current_time + 1 - current_proc->arrive_time;
                completed_procs++;
                current_proc = NULL;
            } else if (algorithm == RR && quantum_counter == quantum) {
                enqueue(current_proc); // Torna a la cua
                current_proc = NULL;
            } 
        }
        
        current_time++;
        if (current_time >= duration) break;
    }

    printSimulation(nprocs, procTable, current_time);
    printMetrics(current_time, nprocs, procTable);
    
    for (int p = 0; p < nprocs; p++) {
        destroyProcess(procTable[p]);
    }
    cleanQueue();
    
    return EXIT_SUCCESS;
}

void printSimulation(size_t nprocs, Process *procTable, size_t duration)
{

    printf("%14s", "== SIMULATION ");
    for (int t = 0; t < duration; t++)
    {
        printf("%5s", "=====");
    }
    printf("\n");

    printf("|%4s", "name");
    for (int t = 0; t < duration; t++)
    {
        printf("|%2d", t);
    }
    printf("|\n");

    for (int p = 0; p < nprocs; p++)
    {
        Process current = procTable[p];
        printf("|%4s", current.name);
        for (int t = 0; t < duration; t++)
        {
            printf("|%2s", (current.lifecycle[t] == Running ? "E" : current.lifecycle[t] == Bloqued ? "B"
                                                                : current.lifecycle[t] == Finished  ? "F"
                                                                                                    : " "));
        }
        printf("|\n");
    }
}

void printMetrics(size_t simulationCPUTime, size_t nprocs, Process *procTable)
{

    printf("%-14s", "== METRICS ");
    for (int t = 0; t < simulationCPUTime + 1; t++)
    {
        printf("%5s", "=====");
    }
    printf("\n");

    printf("= Duration: %ld\n", simulationCPUTime);
    printf("= Processes: %ld\n", nprocs);

    size_t baselineCPUTime = getTotalCPU(procTable, nprocs);
    double throughput = (double)nprocs / (double)simulationCPUTime;
    double cpu_usage = (double)simulationCPUTime / (double)baselineCPUTime;

    printf("= CPU (Usage): %lf\n", cpu_usage * 100);
    printf("= Throughput: %lf\n", throughput * 100);

    double averageWaitingTime = 0;
    double averageResponseTime = 0;
    double averageReturnTime = 0;
    double averageReturnTimeN = 0;

    for (int p = 0; p < nprocs; p++)
    {
        averageWaitingTime += procTable[p].waiting_time;
        averageResponseTime += procTable[p].response_time;
        averageReturnTime += procTable[p].return_time;
        averageReturnTimeN += procTable[p].return_time / (double)procTable[p].burst;
    }

    printf("= averageWaitingTime: %lf\n", (averageWaitingTime / (double)nprocs));
    printf("= averageResponseTime: %lf\n", (averageResponseTime / (double)nprocs));
    printf("= averageReturnTimeN: %lf\n", (averageReturnTimeN / (double)nprocs));
    printf("= averageReturnTime: %lf\n", (averageReturnTime / (double)nprocs));
}