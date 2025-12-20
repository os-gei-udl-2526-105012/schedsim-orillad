#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include "process.h"
#include <string.h>

Process** queue;
size_t elements;

void init_queue(){
    queue = malloc(1 * sizeof(Process*));
    elements = 0;
}

size_t get_queue_size(){
    return elements;
}

int enqueue(Process* process){
    if (queue != NULL){
        queue[elements] = process;
        elements++;
        queue = realloc(queue,(elements+1)*sizeof(Process*));
        queue[elements]=NULL;
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}

Process* peek(void){
    if (elements <= 0){
        return NULL;
    }
    return queue[0];
}

Process* dequeue(){
    if (elements <= 0){
        return NULL;
    } else {
        Process* process = queue[0];
        for (int p = 1; p < elements; p++) {
            queue[p-1] = queue[p];
        }
        elements--;
        queue = realloc(queue, (elements + 1) * sizeof(Process*));
        queue[elements] = NULL;
        return process;
    }
}

void cleanQueue(){
    free(queue);
}

char* queueToString(){
    char* queue_str = malloc( sizeof(char) * strlen("Front->[")+1);
    strcpy(queue_str,"Front->[");
    if (queue != NULL && elements>=1){
        for(int i=0; i<elements-1; i++){
            Process p = *queue[i];
            queue_str = realloc(queue_str, sizeof(char) * (strlen(queue_str) + strlen(p.name) + 2) );
            strcat(queue_str,p.name);
            strcat(queue_str,",");
        }
        Process p = *queue[elements-1];
        queue_str = realloc(queue_str,sizeof(char) * (strlen(queue_str)+strlen(p.name)+1) );
        strcat(queue_str,p.name);   
    }
    queue_str = realloc(queue_str,sizeof(char) * (strlen(queue_str)+strlen("]<-Rear")+1) );
    strcat(queue_str,"]<-Rear");
    return queue_str;
}

Process* transformQueueToList(){
    if (elements == 0) {
        return NULL;
    }
    Process* _list = malloc(elements * sizeof(Process));
    for (int i = 0; i < elements; i++){
        _list[i] = *queue[i];
    }
    return _list; 
}

void setQueueFromList(Process* list){
    if (list == NULL || elements == 0) {
        return;
    }
    Process **new_queue = malloc((elements + 1) * sizeof(Process*));
    size_t new_elements = 0;

    for (size_t i = 0; i < elements; i++) {
        for (size_t j = 0; j < elements; j++) {
            if (queue[j]->id == list[i].id) {
                *(queue[j]) = list[i];
                new_queue[new_elements++] = queue[j];
                break;
            }
        }
    }

    new_queue[new_elements] = NULL;
    free(queue);
    queue = new_queue;
    elements = new_elements;
}




