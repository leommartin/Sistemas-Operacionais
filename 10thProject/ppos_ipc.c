#include "ppos.h"
#include <stdio.h>
#include <stdlib.h>

int lock = 0;

int TSL()
{
    int old = lock;
    lock = 1;
    return old;
}

void enter()
{
    while(TSL()){}
    
    #ifdef DEBUG
        printf("Task %d turn!\n", current_task->id);
    #endif
}

void leave()
{
    lock = 0;
    #ifdef DEBUG
        printf("Task %d out!\n", current_task->id);
    #endif
}

// inicializa um semáforo com valor inicial "value"
int sem_init (semaphore_t *s, int value) 
{   
    #ifdef DEBUG
        printf("\n------------- SEMAPHORE ------------- \n");
    #endif

    if (s == NULL)
    {
        return -1;
    }

    s->counter = value;
    s->queue = NULL;

    #ifdef DEBUG
        printf("--DEGUG: sem_init: semaphore initialized with success\n\n");
    #endif
    return 0;
}

// Requisita o semáforo
/* 
    Decrementa o contador do semáforo.
    Se < 0, suspende a tarefa e a põe na fila de espera do semáforo.
*/
int sem_down (semaphore_t *s) 
{

    if (s == NULL)
    {
        return -1;
    }

    enter();
    s->counter--;
    leave();
    
    if (s->counter < 0)
    {
        #ifdef DEBUG
            printf("--DEGUG: sem_down: task %d suspended and go to semaphore queue.\n", current_task->id);
            print_task_queue((queue_t*)s->queue);
        #endif  

        task_suspend(&s->queue); // To do: verificar
    }

    return 0;
}

// Libera o semáforo
/* 
    Incrementa o contador do semáforo.
    Se <= 0, acorda uma das tarefas da fila de espera do semáforo.
*/
int sem_up (semaphore_t *s) 
{

    if (s == NULL)
    {
        return -1;
    }

    enter();
    s->counter++;
    leave();

    if (s->counter <= 0)
    {
        // printf("\nFila do semaforo antes de acordar: ");
        // print_task_queue((queue_t*)s->queue);

        if(s->queue != NULL)
            task_awake(s->queue, &s->queue);

        #ifdef DEBUG
            printf("--DEGUG: sem_down: task %d reactivated and go to READY tasks queue.\n", current_task->id);
            print_task_queue((queue_t*)s->queue);
            print_task_queue(ready_tasks_queue);
        #endif  
    }


    return 0;
}

// "destroi" o semáforo, liberando as tarefas bloqueadas
int sem_destroy (semaphore_t *s) 
{
    if (s == NULL)
    {
        return -1;
    }

    // printf("\nFila do semaforo antes de destruir: ");
    // print_task_queue((queue_t*)s->queue);
    // printf("Fila de tarefas prontas antes de destruir: ");
    // print_task_queue(ready_tasks_queue);

    while (s->queue != NULL)
    {
        // #ifdef DEBUG
        //     printf("--DEGUG: sem_destroy(): task %d removed of the semaphore queue and added to READY taks queue.\n", s->queue->id);
        // // #endif
        task_awake(s->queue, &s->queue);
    } 

    // printf("Fila de tarefas prontas depois de destruir: ");
    // print_task_queue(ready_tasks_queue);
    // printf("\n");

    enter();
    s->queue = NULL;
    s = NULL;
    leave();
    return 0;
}
