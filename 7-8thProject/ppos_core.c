#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include "ppos.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */
#define ID_MAIN 0
#define SYSTEM_TASK 0
#define USER_TASK 1
#define READY 2
#define RUNNING 3
#define SUSPEND 4
#define TERMINATED 5
#define QUANTUM 10

int id = 0;
int userTasks = -1;     // -1 because main_task and dispatcher are not user tasks
int program_timer = 0; 

// structure that defines an action handler (must be global or static)
struct sigaction action ;

// structure of initialization of the timer
struct itimerval timer;

queue_t *task_queue = NULL;

task_t main_task;
task_t *current_task;
task_t *old_task;
task_t *prox = NULL;
task_t *disp;

struct cpu_timer
{
  unsigned int start_time;
  unsigned int end_time;
};

struct cpu_timer proc_timer;

unsigned int systime()
{
    return program_timer;
}

void print_task_queue() 
{
    if (task_queue == NULL) {
        printf("[ ]\n");
        return;
    }

    queue_t *current = task_queue;
    printf("[ ");

    do {
        task_t *task = (task_t *)current;
        printf("<%d> ", task->id);
        current = current->next;
    } while (current != task_queue);

    printf("]\n");
}

// Verify if the task is a user task 
    // if it is, decrement of quantum   
    // if it is not, nothing to do       
    /* if the quantum timer goes to ZERO, the task that is executing go back to end of the ready queue tasks 
       and the control of CPU returns to dispatcher */
    // if the quantum timer is not ZERO, the task continues executing
void handler (int signum)
{
    if(current_task->type == USER_TASK)
    {
        if(current_task->quantum_timer >= 0)
        {
            current_task->quantum_timer--;
        }

        if(current_task->quantum_timer < 0)
        {
            task_yield();
        }
    }

    program_timer++;  
}

void set_handler()
{
    action.sa_handler = handler;
    sigemptyset (&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction (SIGALRM, &action, 0) < 0)
    {
        perror ("Erro em sigaction: ");
        exit (1);
    }
}

void set_timer()
{
    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1000;      // primeiro disparo, em micro-segundos (1000mcs := 1ms)
    timer.it_value.tv_sec  = 0;      // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000;   // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0 ;   // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL
    if (setitimer (ITIMER_REAL, &timer, 0) < 0)
    {
        perror ("Error at setitimer: ");
        exit (1);
    }
}

void set_dynamic_priorities()
{
    queue_t *queue_aux;
    task_t *task_aux;
    
    // Set all dynamic priorities

    #ifdef DEBUG
        print_task_queue();
        printf("\n");
    #endif

    queue_aux = task_queue;
    while(queue_aux->next != task_queue)
    {
        task_aux = (task_t*) queue_aux;
        task_aux->dynamic_prio = task_aux->static_prio;
        
        #ifdef DEBUG
            printf ("--DEGUG: setting dyn prio to task : %d, dyn_prio:%d \n", task_aux->id, task_aux->dynamic_prio);
        #endif
        
        queue_aux = queue_aux->next;
    }

    task_aux = (task_t*) queue_aux;
    task_aux->dynamic_prio = task_aux->static_prio;
    
    #ifdef DEBUG  
        printf ("--DEGUG: setting dyn prio to task : %d, dyn_prio:%d \n\n", task_aux->id, task_aux->dynamic_prio);
    #endif
}

task_t* scheduler()
{
    queue_t *queue_aux;
    task_t *task_aux, *next_task;
    int highest_prio = 21;

    // if(task_queue == NULL)
    // {
    //     fprintf(stderr, "--ERROR: The head of queue == NULL.");
    //     exit(0);
    // }
    
    #ifdef DEBUG
        printf ("--DEGUG: Traversing the queue to find the highest dynamic priority\n");
    #endif

    int q_size = queue_size(task_queue);

    queue_aux = task_queue;
    task_aux = (task_t*) queue_aux;

    next_task = task_aux; // head of the queue
    int i = 0;
    while(i < q_size)
    {       
        #ifdef DEBUG
                printf ("--DEGUG: Task %d, dyn_prio:%d \n", task_aux->id, task_aux->dynamic_prio) ;
        #endif

        if(task_aux->dynamic_prio < highest_prio)
        {
            next_task = task_aux;
            highest_prio = task_aux->dynamic_prio;

            // #ifdef DEBUG
            //     printf ("--DEGUG: HIGHEST prio to task : %d, dyn_prio:%d \n", next_task->id, next_task->dynamic_prio) ;
            // #endif
        }

        queue_aux = queue_aux->next;
        task_aux = (task_t*) queue_aux;
        i++;
    }

    #ifdef DEBUG
        printf("--DEGUG: next task is : %d, with higher dyn_prio:%d \n\n", next_task->id, next_task->dynamic_prio) ;
        printf("--DEBUG: Setting the new dynamic priorities to the other tasks...\n");
    #endif

    queue_aux = task_queue;
    task_aux = (task_t*) queue_aux;
    i = 0;
    while(i < q_size)
    {        
        if(task_aux->dynamic_prio > 20)
        {
            perror("  --ERROR: Task with invalid priority!\n");
            exit(0);
        }

        if((task_aux->dynamic_prio > -20) && (task_aux != next_task))
        {
            task_aux->dynamic_prio--;
            #ifdef DEBUG
                printf ("--DEGUG: new dyn prio to task: %d, dyn_prio:%d \n", task_aux->id, task_aux->dynamic_prio);
            #endif
        }

        queue_aux = queue_aux->next;
        task_aux = (task_t*) queue_aux;
        i++;
    }

    #ifdef DEBUG
        printf ("\n--DEGUG: changing context to the task: %d with highest dyn prio, dyn_prio:%d \n", next_task->id, next_task->dynamic_prio) ;
    #endif

    next_task->dynamic_prio = next_task->static_prio;
    
    #ifdef DEBUG
        printf ("--DEGUG: resetting dyn prio of task: %d, dyn_prio:%d \n", next_task->id, next_task->dynamic_prio);
    #endif

    return next_task;
};

void dispatcher()
{   
    // prox = disp->next;
    
    // Remove dispatcher of the READY queue tasks
    #ifdef DEBUG
        print_task_queue();
    #endif
    
    queue_remove(&task_queue, (queue_t*)disp);
    
    #ifdef DEBUG
        printf ("--DEGUG: dispatcher(): removing dispatcher of the queue\n") ;
        print_task_queue();
        printf("\n");
    #endif

    set_dynamic_priorities();

    // Change order of the choose of next task
    
    while(userTasks > 0)
    {
        // Scheduler choose the next task to execute
        current_task = disp;
        
        prox = scheduler();
        if(prox != NULL)
        {
            prox->quantum_timer = QUANTUM;
            // Add processing time to dispacther before switching to the next task 
            proc_timer.end_time = systime();
            current_task->processing_time += proc_timer.end_time - proc_timer.start_time;
            task_switch(prox);
        }

        old_task = prox;
        switch (old_task->status)
        {
            case TERMINATED:               
                // queue_remove(&task_queue, (queue_t*) old_task);
                free(old_task->context.uc_stack.ss_sp);
                old_task->context.uc_stack.ss_size = 0;
                userTasks--;
                break;

            default: 
                break;
        }
    }

    prox = disp;
    task_exit(0);

}

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init ()
{   

    // main_task = malloc(sizeof(task_t));
    // if (main_task == NULL) {
    //     perror("Erro na alocação de memória para a tarefa");
    //     exit(1);
    // }
    task_init(&main_task, NULL, NULL);
    print_task_queue();
    
    current_task = &main_task;
    current_task->num_activations++;

    disp = malloc(sizeof(task_t));
    if (disp == NULL) {
        perror("Erro na alocação de memória para a tarefa");
        exit(1);
    }
    task_init(disp, dispatcher, NULL);

    set_handler();
    set_timer();

    print_task_queue();

    setvbuf (stdout, 0, _IONBF, 0);
}

// gerência de tarefas =========================================================

// Inicializa uma nova tarefa. Retorna um ID> 0 ou erro.
// (descritor da nova tarefa, funcao corpo da tarefa, argumentos para a tarefa)
int task_init (task_t *task, void  (*start_func)(void *), void   *arg)
{
    char *stack;

    getcontext (&(task->context));

    stack = malloc (STACKSIZE) ;
    if (stack)
    {
        task->context.uc_stack.ss_sp = stack;
        task->context.uc_stack.ss_size = STACKSIZE ;
        task->context.uc_stack.ss_flags = 0 ;
        task->context.uc_link = 0 ;
    }
    else
    {
        perror ("Erro na criação da pilha: ") ;
        exit (1) ;
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

    makecontext (&(task->context),(void (*)())start_func, 1, (char *)arg);
    queue_append(&task_queue, (queue_t*)task);

    #ifdef DEBUG
        printf ("--DEGUG: task_init: task %d initialized\n", task->id) ;
        print_task_queue();
    #endif

    return 0;
}			

// alterna a execução para a tarefa indicada
int task_switch(task_t *task)
{
    old_task = current_task;
    current_task = task;
    current_task->status = RUNNING;

    if(task->id > 1)
    {
        queue_remove(&task_queue, (queue_t*)task);

        #ifdef DEBUG
            printf ("--DEGUG: Task: %d removed of the queue of ready tasks, status: RUNNING\n", task->id);
            print_task_queue();
            printf("\n");
        #endif
    }

    #ifdef DEBUG
            printf ("--DEGUG: task that wins the CPU: %d\n", task->id) ;
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
        exit(0);
    }
    else
    {   
        #ifdef DEBUG
            printf ("--DEGUG: task %d exit\n", current_task->id);
        #endif
        current_task->status = TERMINATED;

        task_switch(disp);
    }
}

// a tarefa atual libera o processador para outra tarefa
void task_yield () 
{
    // Put the current task at the end of the queue
    // Change the status of the current task to READY.
    queue_append(&task_queue, (queue_t*) current_task);
    #ifdef DEBUG
        printf ("--DEGUG: task_yield(): adding task %d in the queue\n", current_task->id) ;
        print_task_queue();
        printf("\n");
    #endif
    
    current_task->status = READY;

    // Return the CPU to dispatcher
    #ifdef DEBUG
        printf ("--DEGUG: backing to dispatcher... \n");
    #endif

    proc_timer.end_time = systime();
    current_task->processing_time += proc_timer.end_time - proc_timer.start_time;
    task_switch(disp);
}

// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio) 
{
    if(task == NULL)
    {
        current_task->static_prio = prio;
    }
    else
    {
        task->static_prio = prio;
    }
}

// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task) 
{
    if(task == NULL)
    {
        return current_task->static_prio;
    }
    else
    {
        return task->static_prio;
    }
}

/*
    Acorda uma tarefa que está suspensa em uma dada fila, através das seguintes ações:

    se a fila queue não for nula, retira a tarefa apontada por task dessa fila;
    ajusta o status dessa tarefa para “pronta”;
    insere a tarefa na fila de tarefas prontas;
    continua a tarefa atual (não retorna ao dispatcher)
*/
// acorda a tarefa indicada,
// trasferindo-a da fila "queue" para a fila de prontas
void task_awake (task_t *task, task_t **queue)
{
    // se a fila queue não for nula, retira a tarefa apontada por task dessa fila
    if(queue != NULL)
    {
        queue_remove((queue_t**)queue, (queue_t*)task);
    }

    // ajusta o status dessa tarefa para “pronta”
    task->status = READY;

    // insere a tarefa na fila de tarefas prontas
    queue_append(&task_queue, (queue_t*)task);
}


/*
    Suspende a tarefa atual através das seguintes ações:

    retira a tarefa atual da fila de tarefas prontas (se estiver nela);
    ajusta o status da tarefa atual para “suspensa”;
    insere a tarefa atual na fila apontada por queue (se essa fila não for nula);
    retorna ao dispatcher.
*/
// suspende a tarefa atual,
// transferindo-a da fila de prontas para a fila "queue"
void task_suspend (task_t **queue)
{
    // insere a tarefa atual na fila apontada por queue (se essa fila não for nula)
    if(queue != NULL)
    {
        queue_append((queue_t**) queue, (queue_t*) current_task);
    }

    // ajusta o status da tarefa atual para “suspensa”
    current_task->status = SUSPEND;

    //  retira a tarefa atual da fila de tarefas prontas (se estiver nela)
    if(current_task->id > 1)
    {
        queue_remove(&task_queue, (queue_t*)current_task);
    }

    task_switch(disp);
}

/*
    A chamada task_wait (b) faz com que a tarefa atual (corrente) seja suspensa até a conclusão
    da tarefa b. Mais tarde, quando a tarefa b encerrar (usando a chamada task_exit), a tarefa
    suspensa deve retornar à fila de tarefas prontas. Lembre-se que várias tarefas podem ficar
    aguardando que a tarefa b encerre, então todas elas têm de ser acordadas quando isso ocorrer.

    Caso a tarefa b não exista ou já tenha encerrado, esta chamada deve retornar imediatamente, 
    sem suspender a tarefa atual.

    O valor de retorno da chamada task_wait deve ser o código de encerramento da tarefa b
    (valor exit_code informado como parâmetro de task_exit), ou -1, caso a tarefa indicada não exista ou algum outro erro. 
*/

// a tarefa corrente aguarda o encerramento de outra task
int task_wait (task_t *task)
{
    if(task == NULL)
        return -1;

    task_suspend(&current_task);

    return task->id;
}




