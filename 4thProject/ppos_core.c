#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "queue.h"
#include <ucontext.h>

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */
#define ID_MAIN 0
#define NEW 1
#define READY 2
#define RUNNING 3
#define SUSPEND 4
#define TERMINATED 5

int id = 0;
int userTasks = -1; 

queue_t *task_queue = NULL;

task_t main_task;
task_t *current_task;
task_t *old_task;
task_t *prox = NULL;
task_t *disp;

void set_dynamic_priorities()
{
    queue_t *queue_aux;
    task_t *task_aux;
    
    // Set all dynamic priorities

    #ifdef DEBUG
        queue_print("--DEBUG: Queue ", task_queue, print_elem);
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

    if(task_queue == NULL)
    {
        fprintf(stderr, "--ERROR: The head of queue == NULL.");
        exit(0);
    }
    
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

        if(task_aux->dynamic_prio <= highest_prio)
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
}

void dispatcher()
{
    prox = disp->next;
    
    // Remove dispatcher of the READY queue tasks
    #ifdef DEBUG
        queue_print("\n--DEBUG: Queue ", task_queue, print_elem);
    #endif
    
    queue_remove(&task_queue, (queue_t*)disp);
    
    #ifdef DEBUG
        printf ("--DEGUG: dispatcher(): removing dispatcher of the queue\n") ;
        queue_print("--DEBUG: Queue ", task_queue, print_elem);
        printf("\n");
    #endif

    set_dynamic_priorities();
    
    while(userTasks > 0)
    {
        // Scheduler choose the next task to execute
        current_task = disp;

        prox = scheduler();
        if(prox != NULL)
        {
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

    // queue_print("Queue ", task_queue, print_elem);
    prox = disp;
    task_exit(0);

    // When the task comes to an end, the control return to dispatcher
    // and it destroy the task (free the memory allocated)
}

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init ()
{
    main_task.id = ID_MAIN; 
    main_task.prev = NULL;
    main_task.next = NULL; 
    
    current_task = &main_task;

    disp = malloc(sizeof(task_t));
    if (disp == NULL) {
        perror("Erro na alocação de memória para a tarefa");
        exit(1);
    }

    task_init(disp, dispatcher, NULL);

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
    
    id++;
    task->id = id;
    task->status = READY;
    userTasks++;
    task_setprio(task, 0);
    task->dynamic_prio = 0;

    makecontext (&(task->context),(void (*)())start_func, 1, (char *)arg);
    queue_append(&task_queue, (queue_t*)task);

    #ifdef DEBUG
        printf ("--DEGUG: task_init: task %d initialized\n", task->id) ;
        queue_print("--DEBUG: Queue ", task_queue, print_elem);
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
            queue_print("--DEBUG: Queue ", task_queue, print_elem);
            printf("\n");
        #endif
    }

    #ifdef DEBUG
            printf ("--DEGUG: task that wins the CPU: %d\n", task->id) ;
    #endif    
    // Troca o contexto para a tarefa indicada
    swapcontext(&(old_task->context), &(task->context));

    return 0;
}

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id() 
{
    return current_task->id; 
}

// Termina a tarefa corrente com um status de encerramento
void task_exit (int exit_code) 
{
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
        queue_print("--DEBUG: Queue ", task_queue, print_elem);
        printf("\n");
    #endif
    
    current_task->status = READY;

    // Return the CPU to dispatcher
    #ifdef DEBUG
        printf ("--DEGUG: backing to dispatcher... \n");
    #endif
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