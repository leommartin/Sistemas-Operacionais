#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "queue.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */
#define ID_MAIN 0
#define NEW 1
#define READY 2
#define RUNNING 3
#define SUSPEND 4
#define TERMINATED 5

// task_t *main_task;
int id = 1;
int userTasks = 0; // size of the queue = number of tasks created

// task_t main_task;
queue_t *task_queue = NULL;

task_t *current_task;
task_t *old_task;
task_t *disp;


task_t* scheduler()
{
    // returns a pointer to the next task to be executed

    task_t* prox;

    prox = current_task->next;

    return prox;
}

void dispacher()
{
    // Remove dispatcher of the READY queue tasks
    queue_remove(&task_queue, (queue_t*)disp);
    // To do: verify this call of queue_remove

    while(userTasks > 0)
    {
        // Scheduler choose the next task to execute
        old_task = current_task;
        task_t* prox = scheduler();

        if(prox != NULL)
            task_switch(prox);
        else
        {
            queue_remove(&task_queue, (queue_t*)old_task);
            free(old_task->context.uc_stack.ss_sp);
            old_task->context.uc_stack.ss_size = 0;
            task_exit(0);
        }
            
    }

    // When the task comes to an end, the control return to dispatcher
    // and it destroy the task (free the memory allocated)

    // Switch between the tasks

    // task_exit(); call exit?!
}

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init ()
{
    // main_task.id = ID_MAIN;
    // main_task.status = RUNNING;  

    task_init(disp, dispacher, "Dispatcher");

    current_task = disp;

    setvbuf (stdout, 0, _IONBF, 0);
}

// gerência de tarefas =========================================================

// Inicializa uma nova tarefa. Retorna um ID> 0 ou erro.
// (descritor da nova tarefa, funcao corpo da tarefa, argumentos para a tarefa)
int task_init (task_t *task, void  (*start_func)(void *), void   *arg)
{
    char *stack;

    getcontext (&(task->context));
   // Captura o contexto atual e o armazena em ContextPing

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

   return 0;
}			

// alterna a execução para a tarefa indicada
int task_switch(task_t *task)
{
    // current_task->status = READY;
    old_task = current_task;
    old_task->status = TERMINATED;
    userTasks--;

    queue_remove((&task_queue, (*queue_t)old_task));
    free(old_task->context.uc_stack.ss_sp);
    old_task->context.uc_stack.ss_size = 0;

    current_task = task;
    current_task->status = RUNNING;

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
    task_switch(&main_task);
    main_task.status = TERMINATED;
}

// a tarefa atual libera o processador para outra tarefa
void task_yield () 
{
    // Put the current task at the end of the queue
    // Change the status of the current task to READY.
    queue_append(&task_queue, (queue_t*) current_task);
    current_task->status = READY;

    // Return the CPU to dispatcher
    task_switch(disp);
}