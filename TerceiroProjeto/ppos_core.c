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

int id = 1;
int userTasks = -1; 

queue_t *task_queue = NULL;

task_t main_task;
task_t *current_task;
task_t *old_task;
task_t* prox;
task_t *disp;

task_t* scheduler()
{
    // returns a pointer to the next task to be executed
    return prox->next;

    // ponteiro nulo
}

void dispatcher()
{
    prox = disp->next;
    
    // Remove dispatcher of the READY queue tasks
    queue_remove(&task_queue, (queue_t*)disp);
    // queue_print("Fila ", task_queue, print_elem);
    // fprintf(stdout, "Size of the queue: %d\n", userTasks);
    
    while(userTasks > 0)
    {
        // Scheduler choose the next task to execute
        current_task = disp;
        if(prox != NULL)
            task_switch(prox);

        task_exit(prox->id);
        
        switch (prox->status)
        {
            case READY:
                break;

            case TERMINATED: 
            // To do: It's not entering here               
                queue_remove(&task_queue, (queue_t*)old_task);
                free(old_task->context.uc_stack.ss_sp);
                old_task->context.uc_stack.ss_size = 0;
                userTasks--;
                break;

            default: 
                // RUNNING
                // To do: Check the task to change to the status TERMINATED
                break;
        }

        prox = scheduler();
    }

    prox = disp;
    task_exit(0);

    // When the task comes to an end, the control return to dispatcher
    // and it destroy the task (free the memory allocated)

}

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init ()
{
    main_task.id = ID_MAIN;
    main_task.status = RUNNING; 
    main_task.prev = NULL;
    main_task.next = NULL; 
    
    current_task = &main_task;

    // disp->prev = NULL;
    // disp->next = NULL;

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

    task->id = id++;
    task->status = READY;
    userTasks++;

    makecontext (&(task->context),(void (*)())start_func, 1, (char *)arg);
    queue_append(&task_queue, (queue_t*)task);
    // queue_print("Fila ", task_queue, print_elem);

    return 0;
}			

// alterna a execução para a tarefa indicada
int task_switch(task_t *task)
{
    old_task = current_task;
    current_task = task;
    // current_task->status = RUNNING;

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
        exit(0);
    }
    else
    {
        task_switch(disp);
    }
}

// a tarefa atual libera o processador para outra tarefa
void task_yield () 
{
    // Put the current task at the end of the queue
    // Change the status of the current task to READY.
    // queue_append(&task_queue, (queue_t*) current_task);
    // current_task->status = READY;

    // Return the CPU to dispatcher
    task_switch(disp);
}