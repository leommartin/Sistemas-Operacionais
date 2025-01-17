#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include "ppos.h"

#define STACKSIZE 64 * 1024 /* tamanho de pilha das threads */
#define ID_MAIN 0
#define SYSTEM_TASK 0
#define USER_TASK 1
#define READY 2
#define RUNNING 3
#define SUSPEND 4
#define TERMINATED 5
#define QUANTUM 10

int id = 0;
int userTasks = -2; // -1 because main_task and dispatcher are not user tasks
int program_timer = 0;
int id_aux = 0;

// structure that defines an action handler (must be global or static)
struct sigaction action;

// structure of initialization of the timer
struct itimerval timer;

queue_t *ready_tasks_queue = NULL;
task_t *sleeping_tasks_queue = NULL;

task_t main_task;
task_t *current_task;
task_t *old_task;
task_t *prox = NULL;
task_t *disp;

cpu_timer proc_timer;

unsigned int systime()
{
    return program_timer;
}

void print_task_queue(queue_t *tasks_queue)
{
    if (tasks_queue == NULL)
    {
        printf("[ ]\n");
        return;
    }

    queue_t *current = tasks_queue;
    printf("[ ");

     do {
        task_t *task = (task_t *)current;
        printf("<%d> ", task->id);
        current = current->next;
    } while (current != tasks_queue);
    printf("]\n");

}

void handler(int signum)
{
    if (current_task->type == USER_TASK)
    {
        if (current_task->quantum_timer >= 0)
        {
            current_task->quantum_timer--;
        }

        if (current_task->quantum_timer < 0)
        {
            task_yield();
        }
    }

    program_timer++;
    // printf("program_timer: %d\n", program_timer);
}

void set_handler()
{
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGALRM, &action, 0) < 0)
    {
        perror("Erro em sigaction: ");
        exit(1);
    }
}

void set_timer()
{
    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1000;    // primeiro disparo, em micro-segundos (1000mcs := 1ms)
    timer.it_value.tv_sec = 0;        // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000; // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec = 0;     // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL
    if (setitimer(ITIMER_REAL, &timer, 0) < 0)
    {
        perror("Error at setitimer: ");
        exit(1);
    }
}

void set_dynamic_priorities()
{
    queue_t *queue_aux;
    task_t *task_aux;

    // Set all dynamic priorities

    #ifdef DEBUG
        print_task_queue(ready_tasks_queue);
        printf("\n");
    #endif

    queue_aux = ready_tasks_queue;
    while (queue_aux->next != ready_tasks_queue)
    {
        task_aux = (task_t *)queue_aux;
        task_aux->dynamic_prio = task_aux->static_prio;

    #ifdef DEBUG
        printf("--DEGUG: setting dyn prio to task : %d, dyn_prio:%d \n", task_aux->id, task_aux->dynamic_prio);
    #endif

        queue_aux = queue_aux->next;
    }

    task_aux = (task_t *)queue_aux;
    task_aux->dynamic_prio = task_aux->static_prio;

    #ifdef DEBUG
        printf("--DEGUG: setting dyn prio to task : %d, dyn_prio:%d \n\n", task_aux->id, task_aux->dynamic_prio);
    #endif
}

int verify_tasks_to_awake()
{
    int q_size = queue_size((queue_t*)sleeping_tasks_queue);

    task_t *task_aux = sleeping_tasks_queue;
    for(int i = 0; i < q_size; i++)
    {
        // printf("Wake up: %d / Systime: %d\n", task_aux->wake_up_time, systime());
        task_t *next = task_aux->next;
        if (task_aux->wake_up_time <= systime())
        {
            task_awake(task_aux, &sleeping_tasks_queue);
        }

        task_aux = next;
    }

    return q_size;
}

task_t *scheduler()
{
    #ifdef DEBUG
        printf ("--DEGUG: scheduler()\n");
    #endif
    queue_t *queue_aux;
    task_t *task_aux, *next_task;
    int highest_prio = 21;

    if(ready_tasks_queue == NULL)
    {
        fprintf(stderr, "--ERROR: The head of ready tasks queue == NULL.");
        exit(0);
    }

    // #ifdef DEBUG
    //     printf("--DEGUG: Traversing the queue to find the highest dynamic priority\n");
    // #endif

    int q_size = queue_size(ready_tasks_queue);

    queue_aux = ready_tasks_queue;
    task_aux = (task_t *)queue_aux;

    next_task = task_aux; // head of the queue
    int i = 0;
    while (i < q_size)
    {
        // #ifdef DEBUG
        //     printf("--DEGUG: Task %d, dyn_prio:%d \n", task_aux->id, task_aux->dynamic_prio);
        // #endif

        if (task_aux->dynamic_prio < highest_prio)
        {
            next_task = task_aux;
            highest_prio = task_aux->dynamic_prio;

            // #ifdef DEBUG
            //     printf ("--DEGUG: HIGHEST prio to task : %d, dyn_prio:%d \n", next_task->id, next_task->dynamic_prio) ;
            // #endif
        }

        queue_aux = queue_aux->next;
        task_aux = (task_t *)queue_aux;
        i++;
    }

    #ifdef DEBUG
        printf("--DEGUG: next task is : %d, with higher dyn_prio:%d \n\n", next_task->id, next_task->dynamic_prio);
        // printf("--DEBUG: Setting the new dynamic priorities to the other tasks...\n");
    #endif

    queue_aux = ready_tasks_queue;
    task_aux = (task_t *)queue_aux;
    i = 0;
    while (i < q_size)
    {
        if (task_aux->dynamic_prio > 20)
        {
            perror("  --ERROR: Task with invalid priority!\n");
            exit(0);
        }

        if ((task_aux->dynamic_prio > -20) && (task_aux != next_task))
        {
            task_aux->dynamic_prio--;
            // #ifdef DEBUG
                // printf("--DEGUG: new dyn prio to task: %d, dyn_prio:%d \n", task_aux->id, task_aux->dynamic_prio);
            // #endif
        }

        queue_aux = queue_aux->next;
        task_aux = (task_t *)queue_aux;
        i++;
    }

    // #ifdef DEBUG
    //     printf("\n--DEGUG: changing context to the task: %d with highest dyn prio, dyn_prio:%d \n", next_task->id, next_task->dynamic_prio);
    // #endif

    next_task->dynamic_prio = next_task->static_prio;

    // #ifdef DEBUG
    //     printf("--DEGUG: resetting dyn prio of task: %d, dyn_prio:%d \n", next_task->id, next_task->dynamic_prio);
    // #endif

    return next_task;
};

void change_context()
{
    prox = scheduler();
    if (prox != NULL)
    {
        prox->quantum_timer = QUANTUM;
        // Add processing time to dispacther before switching to the next task
        proc_timer.end_time = systime();
        current_task->processing_time += proc_timer.end_time - proc_timer.start_time;
        task_switch(prox);
    }
}

void check_status()
{
    old_task = prox;
    switch (old_task->status)
    {
        case TERMINATED:
            queue_remove(&ready_tasks_queue, (queue_t*) old_task);
            #ifdef DEBUG
                printf ("--DEGUG: dispatcher killing task %d\n", old_task->id);
                print_task_queue(ready_tasks_queue);
            #endif
            free(old_task->context.uc_stack.ss_sp);
            old_task->context.uc_stack.ss_size = 0;
            userTasks--;
            old_task = NULL;
            break;

        default:
            break;
    }
}

void dispatcher()
{
    // Remove dispatcher of the READY queue tasks
    #ifdef DEBUG
        print_task_queue(ready_tasks_queue);
    #endif

    queue_remove(&ready_tasks_queue, (queue_t *)disp);

    #ifdef DEBUG
        printf("--DEGUG: dispatcher(): removing dispatcher of the queue\n");
        print_task_queue(ready_tasks_queue);
        printf("\n");
    #endif
    
    set_dynamic_priorities();

    // Change order of the choose of next task

    while (userTasks > 0)
    {
        // Scheduler choose the next task to execute
        current_task = disp;

        if(verify_tasks_to_awake() < userTasks)
        {
            if(queue_size(ready_tasks_queue) > 0)
            {
                change_context();
                check_status();
            }
        }
    }
    
    // Return to main after all tasks are terminated
    change_context();
    check_status();
    
    // End of the program
    prox = disp;
    task_exit(0);
}

// Init our Operational System
void ppos_init()
{
    setvbuf (stdout, 0, _IONBF, 0);

    task_init(&main_task, NULL, NULL);
    // print_task_queue(ready_tasks_queue);

    current_task = &main_task;
    current_task->num_activations++;
    // current_task->quantum_timer = QUANTUM;

    disp = malloc(sizeof(task_t));
    if (disp == NULL)
    {
        perror("Erro na alocação de memória para a tarefa");
        exit(1);
    }
    task_init(disp, dispatcher, NULL);
    // print_task_queue(ready_tasks_queue);

    set_handler();
    set_timer();
}

// Tasks manegement =========================================================
int task_init(task_t *task, void (*start_func)(void *), void *arg)
{
    char *stack;

    getcontext(&(task->context));

    stack = malloc(STACKSIZE);
    if (stack)
    {
        task->context.uc_stack.ss_sp = stack;
        task->context.uc_stack.ss_size = STACKSIZE;
        task->context.uc_stack.ss_flags = 0;
        task->context.uc_link = 0;
    }
    else
    {
        perror("Erro na criação da pilha: ");
        exit(1);
    }

    task->id = id++;
    task->status = READY;
    userTasks++;
    task_setprio(task, 0);
    task->dynamic_prio = 0;
    task->execution_time = systime();
    task->processing_time = 0;
    task->num_activations = 0;

    task->type = (task->id > 1) ? USER_TASK : SYSTEM_TASK;

    makecontext(&(task->context), (void (*)())start_func, 1, (char *)arg);
    queue_append(&ready_tasks_queue, (queue_t *)task);

#ifdef DEBUG
    printf("--DEGUG: task_init: task %d initialized\n", task->id);
    print_task_queue(ready_tasks_queue);
#endif

    return 0;
}

// alterna a execução para a tarefa indicada
int task_switch(task_t *task)
{
    old_task = current_task;
    current_task = task;
    current_task->status = RUNNING;

    // if (task->id > 1)
    // {
    //     queue_remove(&ready_tasks_queue, (queue_t *)task);

    //     #ifdef DEBUG
    //         printf("--DEGUG: Task: %d removed of the queue of ready tasks, status: RUNNING\n", task->id);
    //         print_task_queue(ready_tasks_queue);
    //         printf("\n");
    //     #endif
    // }

    #ifdef DEBUG
        printf("--DEGUG: task that wins the CPU: %d\n", task->id);
        print_task_queue(ready_tasks_queue);
    #endif

    task->num_activations++;

    proc_timer.start_time = systime();

    // Troca o contexto para a tarefa indicada
    swapcontext(&(old_task->context), &(task->context));

    return 0;
}

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id()
{
    return current_task->id;
}

// Termina a tarefa corrente com um status de encerramentos
void task_exit (int exit_code) 
{
    current_task->execution_time = systime() - current_task->execution_time;

    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", 
    current_task->id, current_task->execution_time, current_task->processing_time, current_task->num_activations);

    if(prox == disp)
    {
        #ifdef DEBUG
            printf ("--DEGUG: dispatcher exit\n");
        #endif
        free(disp->context.uc_stack.ss_sp);
        disp->context.uc_stack.ss_size = 0;
        exit(0);
    }
    else
    {   // Awake tasks that are waiting for the current task

        int q_size = queue_size((queue_t*)current_task->wait_queue);
        task_t *task_aux = current_task->wait_queue;
        for(int i = 0; i < q_size; i++)
        {
            task_t *next = task_aux->next;
            if (task_aux->wake_up_time <= systime())
            {
                task_awake(task_aux, &current_task->wait_queue);
            }

            task_aux = next;
        }
        current_task->status = TERMINATED;
    
        task_switch(disp);
    }
}
// a tarefa atual libera o processador para outra tarefa
void task_yield()
{
    // Put the current task at the end of the queue
    // Change the status of the current task to READY.

    #ifdef DEBUG
        printf("--DEGUG: task_yield(): adding task %d in the queue\n", current_task->id);
        print_task_queue(ready_tasks_queue);
    #endif
    // queue_append(&ready_tasks_queue, (queue_t *)current_task);

    current_task->status = READY;

    // Return the CPU to dispatcher
    #ifdef DEBUG
        printf("--DEGUG: backing to dispatcher... \n");
    #endif

    proc_timer.end_time = systime();
    current_task->processing_time += proc_timer.end_time - proc_timer.start_time;
    task_switch(disp);
}

// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio(task_t *task, int prio)
{
    if (task == NULL)
    {
        current_task->static_prio = prio;
    }
    else
    {
        task->static_prio = prio;
    }
}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio(task_t *task)
{
    if (task == NULL)
    {
        return current_task->static_prio;
    }
    else
    {
        return task->static_prio;
    }
}

// busca por uma tarefa na fila "queue"
// retorna 1 se a tarefa existe e 0 caso contrário
int task_exists(task_t *task, task_t *queue)
{
    task_t *current = queue;

    if(queue == NULL)
        return 0;

    // printf("\n\nReady tasks queue: ");
    // print_task_queue(ready_tasks_queue);

    while (current->next != queue)
    {
        if (current == task)
            return 1;

        current = current->next;
    }

    if (current == task)
        return 1;

    return 0;
}

// acorda a tarefa indicada,
// trasferindo-a da fila "queue" para a fila de prontas
void task_awake(task_t *task, task_t **queue)
{
    #ifdef DEBUG
        printf("\n--DEGUG: task_awake(): Task %d trying to remove T%d of the SLEEPING tasks queue\n", current_task->id, task->id);
        print_task_queue((queue_t*) *queue);
    #endif

    // se a fila queue não for nula, retira a tarefa apontada por task dessa fila
    if (queue != NULL)
    {
        queue_remove((queue_t **)queue, (queue_t *)task);
    }

    #ifdef DEBUG
        printf("--debug: SLEEPING tasks queue: ");
        print_task_queue((queue_t*) *queue);
        printf("\n");
    #endif

    // ajusta o status dessa tarefa para “pronta”
    task->status = READY;

    #ifdef DEBUG
        printf("--DEGUG: task_awake(): trying to add task %d to the READY tasks queue\n", task->id);
        printf("--debug: READY tasks queue: ");
        print_task_queue(ready_tasks_queue);
    #endif
    // insere a tarefa na fila de tarefas prontas
    queue_append(&ready_tasks_queue, (queue_t *)task);

    #ifdef DEBUG
        printf("--debug: READY tasks queue: ");
        print_task_queue(ready_tasks_queue);
        printf("\n");
    #endif
}

// suspende a tarefa atual,
// transferindo-a da fila de prontas para a fila "queue"
void task_suspend (task_t **queue)
{  
    #ifdef DEBUG
        printf("--DEGUG: task_suspend(): trying to remove task %d of the READY tasks queue\n", current_task->id);
        printf("--debug: READY tasks queue: ");
        print_task_queue(ready_tasks_queue);
    #endif
    
    // retira a tarefa atual da fila de tarefas prontas (se estiver nela);
    if(task_exists(current_task, (task_t*)ready_tasks_queue))
        queue_remove(&ready_tasks_queue, (queue_t*)current_task);

    #ifdef DEBUG
        printf("--debug: READY tasks queue: ");
        print_task_queue(ready_tasks_queue);
        printf("\n");
    #endif
    // #ifdef DEBUG
    //     printf("--DEGUG: task_suspend(): after task %d being removed of the READY tasks queue\n", current_task->id);
    //     print_task_queue(ready_tasks_queue);
    //     printf("\n");
    // #endif

    // ajusta o status da tarefa atual para “suspensa”
    current_task->status = SUSPEND;

    #ifdef DEBUG
        printf("\n--DEGUG: task_suspend(): adding task %d  to the SLEEPING tasks queue\n", current_task->id);
    #endif

    // insere a tarefa atual na fila apontada por queue (se essa fila não for nula)
    if(queue != NULL)
    {
        queue_append((queue_t**) queue, (queue_t*) current_task);
    }

    #ifdef DEBUG
        if(queue != &sleeping_tasks_queue)
        {
            printf("--debug: SLEEPING tasks queue of task %d: \n", id_aux);
        }
        // else
        // {
        //     printf("--debug: SLEEPING tasks queue: ");
        // }
        print_task_queue((queue_t*) *queue);
        printf("--debug: SLEEPING tasks queue: ");
        print_task_queue((queue_t*)sleeping_tasks_queue);
        printf("\n");
    #endif

    task_switch(disp);
}

// a tarefa corrente aguarda o encerramento de outra task
int task_wait (task_t *task)
{
    if(task == NULL || task->status == TERMINATED)
        return -1;

    id_aux = task->id;
        
    task_suspend(&task->wait_queue);

    return task->id;
}

// suspende a tarefa corrente por t milissegundos
//  calcula o instante em que a tarefa atual deverá ser acordada
// e a suspende na fila de tarefas adormecidas, usando task_suspend
void task_sleep (int t) 
{
    current_task->wake_up_time = systime() + t;

    task_suspend(&sleeping_tasks_queue);
}
