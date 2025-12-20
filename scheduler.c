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
    Process *current_proc[2] = {NULL, NULL};
    int quantum_counter[2] = {0, 0};
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

    while (completed_procs < nprocs) {

        /* Enqueue arrivals */
        while (next_arrival_index < nprocs && procTable[next_arrival_index].arrive_time <= current_time) {
            Process *p = &procTable[next_arrival_index];
            enqueue(p);
            next_arrival_index++;
        }
   
        /* Sort ready queue if needed */
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

        /* Fill idle CPUs */
        for (int i = 0; i < 2 && get_queue_size() > 0; i++) {
            if (current_proc[i] == NULL) {
                current_proc[i] = dequeue();
                quantum_counter[i] = 0;
            }
        }

        /* If everything idle and pending arrivals, jump forward */
        if (current_proc[0] == NULL && current_proc[1] == NULL && get_queue_size() == 0 && next_arrival_index < nprocs) {
            current_time = procTable[next_arrival_index].arrive_time;
            continue;
        }

        /* Increment waiting time in ready queue */
        Process *waiting_list = transformQueueToList();
        if (waiting_list != NULL) {
            size_t queue_size = get_queue_size();
            for (int j = 0; j < (int)queue_size; j++) {
                waiting_list[j].waiting_time++;
            }
            setQueueFromList(waiting_list); 
            free(waiting_list);
        }

        /* Execute each CPU slot */
        for (int i = 0; i < 2; i++) {
            if (current_proc[i] == NULL) {
                continue;
            }

            /* Preemption check for SJRT/priorities */
            if (modality == PREEMPTIVE && get_queue_size() > 0 && (algorithm == SJF || algorithm == PRIORITIES)) {
                Process *candidate = peek();
                if (candidate != NULL) {
                    bool should_preempt = false;
                    if (algorithm == SJF) {
                        int current_remaining = current_proc[i]->burst - getCurrentBurst(current_proc[i], current_time);
                        int next_remaining = candidate->burst - getCurrentBurst(candidate, current_time);
                        if (next_remaining < current_remaining) {
                            should_preempt = true;
                        }
                    } else if (algorithm == PRIORITIES) {
                        if (candidate->priority < current_proc[i]->priority) {
                            should_preempt = true;
                        }
                    }
                    if (should_preempt) {
                        enqueue(current_proc[i]);
                        current_proc[i] = dequeue(); /* candidate */
                        quantum_counter[i] = 0;
                    }
                }
            }

            current_proc[i]->lifecycle[current_time] = Running;

            if (current_proc[i]->response_time == -1) {
                current_proc[i]->response_time = current_time - current_proc[i]->arrive_time;
            }

            int executed_burst = getCurrentBurst(current_proc[i], current_time + 1);
            quantum_counter[i]++;

            if (executed_burst >= current_proc[i]->burst) {
                current_proc[i]->lifecycle[current_time] = Finished;
                current_proc[i]->completed = true;
                current_proc[i]->return_time = current_time + 1 - current_proc[i]->arrive_time;
                completed_procs++;
                current_proc[i] = NULL;
            } else if (algorithm == RR && quantum_counter[i] == quantum) {
                enqueue(current_proc[i]); // Torna a la cua
                current_proc[i] = NULL;
            } 
        }
        
        current_time++;
        if (current_time >= duration) break;
    }

    printSimulation(nprocs, procTable, (size_t)current_time);
    printMetrics((size_t)current_time, nprocs, procTable);
    
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
    double cpu_usage = (double)baselineCPUTime / ((double)simulationCPUTime * 2);

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
