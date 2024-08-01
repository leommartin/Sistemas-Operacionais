#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include "ppos.h"
#include "disk.h"
#include "ppos_disk.h"
#include "ppos_data.h"
#include <string.h>

// #define STACKSIZE 64 * 1024 /* tamanho de pilha das threads */
// #define ID_MAIN 0
// #define SYSTEM_TASK 0
// #define QUEUE_STATUS_DEAD 0
// #define QUEUE_STATUS_ALIVE 1
// #define USER_TASK 1
// #define READY 2
// #define RUNNING 3
// #define SUSPEND 4
// #define TERMINATED 5
// #define QUANTUM 10

int id = 0;
int userTasks = -2; // -1 because main_task and dispatcher are not user tasks
int program_timer = 0;
int id_aux = 0;

// structure that defines an action handler (must be global or static)
struct sigaction action;

// structure of initialization of the timer
struct itimerval timer;

// ready_tasks_queue = NULL;
// disk_tasks_queue = NULL;

task_t main_task;
task_t *old_task;
task_t *prox = NULL;
task_t *disp;

task_t *current_task = NULL;
task_t disk_manager;
queue_t *ready_tasks_queue = NULL;
task_t *sleeping_tasks_queue = NULL;
semaphore_t sem_disk;
task_t *disk_tasks_queue = NULL;

disk_t disk;

cpu_timer proc_timer;

// print print_task_queue(ready_tasks_queue)
// print print_task_queue((queue_t*)sleeping_tasks_queue)

unsigned int systime()
{
    return program_timer;
}

void print_task_queue(queue_t *tasks_queue)
{
    printf("\t");
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

    // #ifdef DEBUG
    //     printf("\t--DEBUG: setting dyn prio to task : %d, dyn_prio:%d \n", task_aux->id, task_aux->dynamic_prio);
    // #endif

        queue_aux = queue_aux->next;
    }

    task_aux = (task_t *)queue_aux;
    task_aux->dynamic_prio = task_aux->static_prio;

    // #ifdef DEBUG
    //     printf("\t--DEBUG: setting dyn prio to task : %d, dyn_prio:%d \n\n", task_aux->id, task_aux->dynamic_prio);
    // #endif
}

int verify_tasks_to_awake()
{
    #ifdef DEBUG
            printf("-> Verificando tarefas para acordar...\n");
    #endif
    int q_size = queue_size((queue_t*)sleeping_tasks_queue);

    task_t *task_aux = sleeping_tasks_queue;
    for(int i = 0; i < q_size; i++)
    {
        // printf("Wake up: %d / Systime: %d\n", task_aux->wake_up_time, systime());
        task_t *next = task_aux->next;
        if (task_aux->wake_up_time <= systime())
        {
            task_awake(task_aux, &sleeping_tasks_queue);
            #ifdef DEBUG
                printf("-> Tarefa %d acordou!\n", task_aux->id);
            #endif
        }

        task_aux = next;
    }

    return q_size;
}

task_t *scheduler()
{
    #ifdef DEBUG
        printf ("\t--DEBUG: scheduler()\n");
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
    //     printf("\t--DEBUG: Traversing the queue to find the highest dynamic priority\n");
    // #endif

    int q_size = queue_size(ready_tasks_queue);

    queue_aux = ready_tasks_queue;
    task_aux = (task_t *)queue_aux;

    next_task = task_aux; // head of the queue
    int i = 0;
    while (i < q_size)
    {
        // #ifdef DEBUG
        //     printf("\t--DEBUG: Task %d, dyn_prio:%d \n", task_aux->id, task_aux->dynamic_prio);
        // #endif

        if (task_aux->dynamic_prio < highest_prio)
        {
            next_task = task_aux;
            highest_prio = task_aux->dynamic_prio;

            // #ifdef DEBUG
            //     printf ("\t--DEBUG: HIGHEST prio to task : %d, dyn_prio:%d \n", next_task->id, next_task->dynamic_prio) ;
            // #endif
        }

        queue_aux = queue_aux->next;
        task_aux = (task_t *)queue_aux;
        i++;
    }

    #ifdef DEBUG
        printf("\t--DEBUG: next task is : %d, with higher dyn_prio:%d \n\n", next_task->id, next_task->dynamic_prio);
        // printf("\t--DEBUG: Setting the new dynamic priorities to the other tasks...\n");
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
                // printf("\t--DEBUG: new dyn prio to task: %d, dyn_prio:%d \n", task_aux->id, task_aux->dynamic_prio);
            // #endif
        }

        queue_aux = queue_aux->next;
        task_aux = (task_t *)queue_aux;
        i++;
    }

    // #ifdef DEBUG
    //     printf("\n\t--DEBUG: changing context to the task: %d with highest dyn prio, dyn_prio:%d \n", next_task->id, next_task->dynamic_prio);
    // #endif

    next_task->dynamic_prio = next_task->static_prio;

    // #ifdef DEBUG
    //     printf("\t--DEBUG: resetting dyn prio of task: %d, dyn_prio:%d \n", next_task->id, next_task->dynamic_prio);
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
                printf ("\t--DEBUG: dispatcher killing task %d\n", old_task->id);
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
        printf("\t--DEBUG: dispatcher(): removing dispatcher of the queue\n");
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
    
    // End of the program
    prox = disp;
    task_exit(0);
}

void disk_signal_handler(int signum) 
{
    // Acorda o gerenciador de disco

    disk.signal = OPERATION_DONE;
    // task_awake(&disk_manager, &sleeping_tasks_queue);
    #ifdef DEBUG
        // printf("\t--DEBUG: disk_signal_handler(): disk.signal = %d\n", disk.signal);
        printf("disk_signal_handler(): disk.signal = %d\n", disk.signal);
    #endif

    // disk.signal = 0;
}

void set_disk_signal_handler()
{
    action.sa_handler = disk_signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGUSR1, &action, NULL) < 0) {
        perror("Erro em sigaction: ");
        exit(1);
    }
}

void diskDriverBody (void * args)
{
    while (1) 
    {
        // obtém o semáforo de acesso ao disco 
        current_task = &disk_manager; 
        current_task->status = RUNNING;

        #ifdef DEBUG
            printf("diskDriverBody(): current_task = %d\n", current_task->id);
        #endif

        sem_down(&sem_disk);

        #ifdef DEBUG
            printf("-> Conseguiu acesso ao semaforo/disco\n");
        #endif

        // se foi acordado devido a um sinal do disco
        if(disk.signal == OPERATION_DONE) // se recebeu sinal de SIGUSR1 essa flag é ativada
        {
            // acorda a tarefa cujo pedido foi atendido
            // #ifdef DEBUG
                // printf("--DISK: Recebeu sinal de SIGUSR1.");
            // #endif
            task_t *task_aux = disk_tasks_queue;
            task_awake(task_aux, &disk_tasks_queue);
            
            // #ifdef DEBUG
                // printf("--DISK: Acordou tarefa %d, cujo pedido foi atendido.\n", task_aux->id);
            // #endif
            disk.signal = 0; // reseta a flag para 0
        }
        
        if(disk_cmd (DISK_CMD_STATUS, 0, 0) == 1 && disk_tasks_queue != NULL)
        {
            // #ifdef DEBUG
            //     printf("-> Tarefas na fila do disco: ");
            //     print_task_queue((queue_t*)disk_tasks_queue);
            //     printf("\n");
            // #endif

            task_t *task_aux = disk_tasks_queue;
            // queue_remove((queue_t**)&disk_tasks_queue, (queue_t*)task_aux);

            // #ifdef DEBUG
            //     printf("--DISK: Removeu tarefa %d da fila do disco para realizar operação de leitura/escrita.\n", task_aux->id);
            // #endif

            if(task_aux->rw_request.type == READ_OPERATION)
            {
                if(disk_cmd(DISK_CMD_READ, task_aux->rw_request.block, task_aux->rw_request.buffer) < 0)
                {
                    perror("Error while reading block from disk\n");
                    exit(1);
                }
                // #ifdef DEBUG
                    // printf("--DISK: Operação de LEITURA realizada.\n");
                // #endif
                // disk.signal = OPERATION_DONE;
            }
            else if(task_aux->rw_request.type == WRITE_OPERATION)
            {
                if(disk_cmd(DISK_CMD_WRITE, task_aux->rw_request.block, task_aux->rw_request.buffer) < 0)
                {
                    perror("Error while writing block to disk\n");
                    exit(1);
                }
                // #ifdef DEBUG
                    // printf("--DISK: Operação de ESCRITA realizada.\n");
                // #endif
                // disk.signal = OPERATION_DONE;
            }
            else
            {
                #ifdef DEBUG
                    printf("--DISK: Nenhuma operação realizada.\n");
                    printf("--ERROR: Verificar setamento de operação de leitura/escrita.\n");
                #endif
            }
            task_aux->rw_request.type = NO_OPERATION;
        }       

        // libera o semáforo de acesso ao disco
        sem_up(&sem_disk);

         #ifdef DEBUG
            printf("-> Liberou acesso ao semaforo/disco\n\n");
        #endif
        
        // suspende a tarefa corrente (retorna ao dispatcher)
        task_suspend(&sleeping_tasks_queue);
        // To do: verificar em que fila a tarefa deve ser colocada.
    }
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

    task_init(&disk_manager, diskDriverBody, NULL);
    print_task_queue(ready_tasks_queue);

    printf("\n\tInicializando semaforo de acesso ao disco...\n");
    sem_init(&sem_disk, 1);

    set_handler();
    set_disk_signal_handler();
    set_timer();
}

// max_msgs: número de chars, floats ou ints que serão enviados em uma mensagem
// msg_size: tamanho de cada mensagem em bytes dependendo do tipo(char, float, int, etc)
int mqueue_init(mqueue_t *queue, int max_msgs, int msg_size)
{
    if(queue == NULL || max_msgs <= 0 || msg_size <= 0)
    {
        return -1;
    }
    
    queue->buffer = NULL;
    queue->max_msgs = max_msgs;
    queue->msg_size = msg_size;
    queue->size = 0;
    queue->status = QUEUE_STATUS_ALIVE;

    sem_init(&queue->s_buffer, 1);
    sem_init(&queue->s_item, 0);
    sem_init(&queue->s_vaga, max_msgs);

    return 0;
}

// envia uma mensagem para a fila
// Envia a mensagem apontada por msg
// para o fim da fila queue; esta chamada é bloqueante: 
// caso a fila esteja cheia, a tarefa corrente é suspensa até que o envio possa ser feito. 
// O ponteiro msg aponta para um buffer contendo a mensagem a enviar, que deve ser copiada 
// para dentro da fila. Retorna 0 em caso de sucesso e -1 em caso de erro.
// Sugestão: copie a mensagem do buffer para a fila (ou vice-versa) usando
// funções C como bcopy ou memcpy. 
int mqueue_send (mqueue_t *queue, void *msg) 
{
    if(queue == NULL || queue->status == QUEUE_STATUS_DEAD)
        return -1;
    
    mqueue_msg_t *msg_item;
    msg_item = malloc(sizeof(mqueue_msg_t));
    if(msg_item == NULL)
        return -1;

    msg_item->prev = NULL;
    msg_item->next = NULL;
    
    msg_item->content = malloc(queue->msg_size);
    if(msg_item->content == NULL)
        return -1;

    memcpy(msg_item->content, msg, queue->msg_size);

    sem_down(&queue->s_vaga);
    sem_down(&queue->s_buffer);
    if(queue->size >= queue->max_msgs)
    {
        sem_up(&queue->s_buffer);
        sem_up(&queue->s_vaga);
        return -1;
    }
    else
    {
        queue_append((queue_t**)&queue->buffer, (queue_t*)msg_item);
        queue->size++;
        sem_up(&queue->s_buffer);
        sem_up(&queue->s_item);
    }

    return 0;
}

// Recebe uma mensagem do início da fila queue e a deposita
// no buffer apontado por msg; esta chamada é bloqueante: 
// caso a fila esteja vazia, a tarefa corrente é suspensa
// até que a recepção possa ser feita. O ponteiro msg 
// aponta para um buffer que irá receber a mensagem. 
// Retorna 0 em caso de sucesso e -1 em caso de erro. 
// recebe uma mensagem da fila
int mqueue_recv (mqueue_t *queue, void *msg) 
{
    if(queue == NULL || queue->status == QUEUE_STATUS_DEAD)
    {
        return -1;
    }

    mqueue_msg_t *msg_item;

    sem_down(&queue->s_item);
    sem_down(&queue->s_buffer);
    msg_item = (mqueue_msg_t*)queue->buffer;
    if(msg_item != NULL && queue->size > 0)
    {
        queue_remove((queue_t**)&queue->buffer, (queue_t*)msg_item);
        queue->size--;
    }
    sem_up(&queue->s_buffer);
    sem_up(&queue->s_vaga);

    if(msg_item == NULL)
        return -1;

    memcpy(msg, msg_item->content, queue->msg_size);
    free(msg_item->content);
    free(msg_item);

    return 0;
}

// Destroi a fila, liberando as tarefas bloqueadas
// Encerra a fila de mensagens indicada por queue, 
// destruindo seu conteúdo e liberando todas as tarefas
// que esperam mensagens dela (essas tarefas devem retornar
// das suas respectivas chamadas com valor de retorno -1). 
// Retorna 0 em caso de sucesso e -1 em caso de erro. 
int mqueue_destroy (mqueue_t *queue)
{
    if(queue == NULL)
    {
        return -1;
    }

    sem_destroy(&queue->s_buffer);
    sem_destroy(&queue->s_item);
    sem_destroy(&queue->s_vaga);

    mqueue_msg_t *msg_item = queue->buffer;
    while(queue->size > 1)
    {
        mqueue_msg_t *next = msg_item->next;
        queue_remove((queue_t**)&queue->buffer, (queue_t*)msg_item);
        free(msg_item->content);
        free(msg_item);
        queue->size--;
        msg_item = next;
    }

    queue->status = 0;
    queue->buffer = NULL;
    queue = NULL;
    free(queue);

    return 0;
}

// informa o número de mensagens atualmente na fila
int mqueue_msgs (mqueue_t *queue) 
{
    return queue->size;
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
    task->wake_up_time = 0;
    task->rw_request.type = NO_OPERATION;

    task->type = (task->id > 1) ? USER_TASK : SYSTEM_TASK;

    makecontext(&(task->context), (void (*)())start_func, 1, (char *)arg);
    queue_append(&ready_tasks_queue, (queue_t *)task);

#ifdef DEBUG
    printf("\t--DEBUG: task_init: task %d initialized\n", task->id);
    print_task_queue(ready_tasks_queue);
#endif

    return 0;
}

// Alterna a execução para a tarefa indicada
int task_switch(task_t *task)
{
    old_task = current_task;
    current_task = task;
    current_task->status = RUNNING;

    // if (task->id > 1)
    // {
    //     queue_remove(&ready_tasks_queue, (queue_t *)task);

    //     #ifdef DEBUG
    //         printf("\t--DEBUG: Task: %d removed of the queue of ready tasks, status: RUNNING\n", task->id);
    //         print_task_queue(ready_tasks_queue);
    //         printf("\n");
    //     #endif
    // }

    #ifdef DEBUG
        printf("\t--DEBUG: task that wins the CPU: %d\n", task->id);
        print_task_queue(ready_tasks_queue);
    #endif

    task->num_activations++;

    proc_timer.start_time = systime();

    // Troca o contexto para a tarefa indicada
    swapcontext(&(old_task->context), &(task->context));

    return 0;
}

// Retorna o identificador da tarefa corrente (main deve ser 0)
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
            printf ("\t--DEBUG: dispatcher exit\n");
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
// A tarefa atual libera o processador para outra tarefa
void task_yield()
{
    // Put the current task at the end of the queue
    // Change the status of the current task to READY.

    #ifdef DEBUG
        printf("\t--DEBUG: task_yield(): adding task %d in the queue\n", current_task->id);
        print_task_queue(ready_tasks_queue);
    #endif
    // queue_append(&ready_tasks_queue, (queue_t *)current_task);

    current_task->status = READY;

    // Return the CPU to dispatcher
    #ifdef DEBUG
        printf("\t--DEBUG: backing to dispatcher... \n");
    #endif

    proc_timer.end_time = systime();
    current_task->processing_time += proc_timer.end_time - proc_timer.start_time;
    task_switch(disp);
}

// Define a prioridade estática de uma tarefa (ou a tarefa atual)
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
        printf("\n\t--DEBUG: task_awake(): Task %d trying to remove T%d of the SLEEPING tasks queue\n", current_task->id, task->id);
        print_task_queue((queue_t*) *queue);
    #endif
    // se a fila queue não for nula, retira a tarefa apontada por task dessa fila
    if (queue != NULL && *queue != NULL)
    {
        if(task_exists(task, (task_t*)*queue))
            queue_remove((queue_t **)queue, (queue_t *)task);
    }

    #ifdef DEBUG
        printf("\t--DEBUG: SLEEPING tasks queue: ");
        print_task_queue((queue_t*) *queue);
        printf("\n");
    #endif

    // ajusta o status dessa tarefa para “pronta”
    task->status = READY;

    #ifdef DEBUG
        printf("\t--DEBUG: task_awake(): trying to add task %d to the READY tasks queue\n", task->id);
        printf("\t--DEBUG: READY tasks queue: ");
        print_task_queue(ready_tasks_queue);
    #endif
    // insere a tarefa na fila de tarefas prontas
    // printf("task_awake() append\n");
    if(!task_exists(task, (task_t*)ready_tasks_queue))
        queue_append(&ready_tasks_queue, (queue_t *)task);

    #ifdef DEBUG
        printf("\t--DEBUG: READY tasks queue: ");
        print_task_queue(ready_tasks_queue);
        printf("\n");
    #endif
}

// suspende a tarefa atual,
// transferindo-a da fila de prontas para a fila "queue"
void task_suspend (task_t **queue)
{  
    // #ifdef DEBUG
        // printf("\t--DEBUG: task_suspend(): trying to REMOVE task %d of the READY tasks queue\n", current_task->id);
        // printf("\t--DEBUG: READY tasks queue: ");
        // print_task_queue(ready_tasks_queue);
    // #endif

    // retira a tarefa atual da fila de tarefas prontas (se estiver nela);
    if(task_exists(current_task, (task_t*)ready_tasks_queue))
    {
        // printf("Removendo tarefa %d da fila de prontas\n", current_task->id);
        // print_task_queue(ready_tasks_queue);
        queue_remove(&ready_tasks_queue, (queue_t*)current_task);
        // print_task_queue(ready_tasks_queue);
    }

    // printf("\t--DEBUG: READY tasks queue: ");
    // print_task_queue(ready_tasks_queue);

    // #ifdef DEBUG
        // printf("\n After remove:\n\t--DEBUG: READY tasks queue: ");
        // print_task_queue(ready_tasks_queue);
        // printf("\n");
    // #endif
    // #ifdef DEBUG
    //     printf("\t--DEBUG: task_suspend(): after task %d being removed of the READY tasks queue\n", current_task->id);
    //     print_task_queue(ready_tasks_queue);
    //     printf("\n");
    // #endif

    // ajusta o status da tarefa atual para “suspensa”
    current_task->status = SUSPEND;

    #ifdef DEBUG
        printf("\n\t--DEBUG: task_suspend(): ADDING task %d  to the SLEEPING tasks queue\n", current_task->id);
    #endif

    // insere a tarefa atual na fila apontada por queue (se essa fila não for nula)
    if(queue != NULL)
    {
        // printf("\t--DEBUG: READY tasks queue: ");
        // print_task_queue(ready_tasks_queue);
        if(task_exists(current_task, (task_t*)ready_tasks_queue))
        {   
            // printf("task_suspend() append\n");
            queue_remove(&ready_tasks_queue, (queue_t*)current_task);
        }
        else
        {
            queue_append((queue_t**) queue, (queue_t*) current_task);
        }
    }

    #ifdef DEBUG
        if(queue == &disk_tasks_queue)
        {
            printf("\n-> Fila do disco:\n ");
            print_task_queue((queue_t*)disk_tasks_queue);
            printf("\n");
        }
    #endif

    // #ifdef DEBUG
    //     if(queue != &sleeping_tasks_queue)
    //     {
    //         printf("\t--DEBUG: SEMAPHORE || WAIT tasks queue: ");
    //         print_task_queue((queue_t*) *queue);  
    //     }
    //     else
    //     {
    //         printf("\t--DEBUG: SLEEPING tasks queue: ");
    //         print_task_queue((queue_t*)sleeping_tasks_queue);
    //         printf("\n");
    //     }
    // #endif

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
