#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

task_t main_task;
task_t current_task;
ucontext_t old_context;


// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init ()
{
    ucontext_t context;

    getcontext(&context);
    current_task.context = context;

    main_task.id = 0;
    main_task.status = 1;

    current_task.id = main_task.id;

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
        // exit (1) ;
        return 1;
   }

   makecontext (&(task->context),(void (*)())start_func, 1, (char *)arg) ;
   return 0;
}			

// alterna a execução para a tarefa indicada
int task_switch (task_t *task) 
{

    old_context = current_task.context;

    if(swapcontext(&old_context, &task->context))
    {
        current_task = *task;
        return 0;
    }
    
    //else
    return -1;
}

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id() 
{
    return current_task.id;
}

// Termina a tarefa corrente com um status de encerramento
void task_exit (int exit_code) 
{
    task_switch(&main_task);
    main_task.status = 0;
}

