#include "ppos.h"
#include "queue.h"
#include "disk.h"
#include "ppos_disk.h"
#include <stdio.h>

void print_tasks_queue(queue_t *tasks_queue)
{
    // printf("\t");
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


// Ao retornar da chamada, a variável num_blocks contém o número de blocos
// do disco inicializado, enquanto a variável block_size contém o tamanho
// de cada bloco do disco, em bytes. Essa chamada retorna 0 em caso de
// sucesso ou -1 em caso de erro.
int disk_mgr_init (int *num_blocks, int *block_size)
{
    if (disk_cmd(DISK_CMD_INIT, 0, 0) < 0)
        return -1;

    *num_blocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    *block_size = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);

    if(*num_blocks < 0 || *block_size < 0)
    {
        return -1;
    }

    disk.num_blocks = *num_blocks;
    disk.block_size = *block_size;

    #ifdef DEBUG
        printf("disk_mgr_init()");
        printf("-> Inicializou o gerente de disco\n");
        printf("-> Número de blocos: %d\n", *num_blocks);
        printf("-> Tamanho do bloco: %d\n\n", *block_size);
    #endif

    return 0;
}

// block: posição (número do bloco) a ler ou escrever no 
// disco (deve estar entre 0 e numblocks-1);
// buffer: endereço dos dados a escrever no disco, ou onde
// devem ser colocados os dados lidos do disco; esse buffer
// deve ter capacidade para block_size bytes.
// retorno: 0 em caso de sucesso ou -1 em caso de erro.
int disk_block_read  (int block, void* buffer) 
{
    if(block < 0 || block >= disk.num_blocks)
    {
        fprintf(stderr, "Error: block out of range\n");
        return -1;
    }
    
    // obtém o semáforo de acesso ao disco
    sem_down(&sem_disk);

    #ifdef DEBUG
        printf("disk_block_read()");
        printf("-> Conseguiu acesso ao semaforo/disco\n");
        printf("-> Tarefas na fila do disco: ");
        print_tasks_queue((queue_t*)disk_tasks_queue);
        printf("\n");
    #endif
 
    // inclui o pedido na fila do disco
    current_task->rw_request.type = READ_OPERATION;
    current_task->rw_request.block = block;  
    // current_task->rw_request.buffer = malloc(disk.block_size); 
    current_task->rw_request.buffer = buffer;

    #ifdef DEBUG
        printf("-> Setou a operação de leitura para solicitar ao disco\n");
    #endif

    // queue_append((queue_t**)disk_tasks_queue, (queue_t*) current_task);
    // To do: não faria mais sentido isso acontecer quando suspendesse a tarefa?

    // -> Se o disco não estiver livre, suspende a tarefa corrente e coloca na fila do disco
    // if(disk_cmd (DISK_CMD_STATUS, 0, 0) != DISK_STATUS_IDLE)
    // {
    //     queue_append((queue_t**)disk_tasks_queue, (queue_t*) current_task);
    // }
    // if(disk_cmd(DISK_CMD_READ, block, buffer) < 0)
    // {
    //     return -1;
    // }
    
    // se o gerente de disco está dormindo
    //   -> acorda o gerente de disco (põe ele na fila de prontas)
    if(disk_manager.status == SUSPEND)
    {
        //  acorda o gerente de disco e põe ele na fila de prontas)
        task_awake(&disk_manager, &sleeping_tasks_queue);
        #ifdef DEBUG
            printf("-> Acordou a tarefa gerenciadora de disco\n");
        #endif
    }
    else
    {
        #ifdef DEBUG
            printf("-> Gerente de disco(<%d>) não está dormindo, status: ", disk_manager.id);
            if(disk_manager.status == READY)
            {
                printf("READY\n");
            }
            else
            {
                printf("RUNNING\n");
            }
            printf("-> Fila de prontas: ");
            print_tasks_queue(ready_tasks_queue);
        #endif
    }
 
    // libera semáforo de acesso ao disco
    sem_up(&sem_disk);

    // suspende a tarefa corrente (retorna ao dispatcher)
    disk.signal = 0;
    
    #ifdef DEBUG
        printf("-> Liberou semaforo, resetou signal e suspendeu a tarefa colocando-a na fila do disco.\n");
    #endif
    
    task_suspend((task_t**)&disk_tasks_queue);

    #ifdef DEBUG
        printf("--DISK: Retornou sinal para tarefa(<%d>) com o TÉRMINO da OPERAÇÃO DE LEITURA.\n", current_task->id);
    #endif

    return 0;
}

int disk_block_write (int block, void* buffer) 
{
    // obtém o semáforo de acesso ao disco
    sem_down(&sem_disk);
    #ifdef DEBUG
        printf("disk_block_write()");
        printf("-> Conseguiu acesso ao semaforo/disco\n");
        printf("-> Tarefas na fila do disco: ");
        print_tasks_queue((queue_t*)disk_tasks_queue);
        printf("\n");
    #endif

    
    // inclui o pedido na fila_disco
    current_task->rw_request.type = READ_OPERATION;
    current_task->rw_request.block = block;  
    // current_task->rw_request.buffer = malloc(disk.block_size); 
    current_task->rw_request.buffer = buffer;
    #ifdef DEBUG
        printf("-> Setou a operação de escrita para solicitar ao disco\n");
    #endif

    // queue_append((queue_t**)disk_tasks_queue, (queue_t*) current_task);
    // To do: não faria mais sentido isso acontecer quando suspendesse a tarefa?
 
    // se o gerente de disco está dormindo
    //   -> acorda o gerente de disco (põe ele na fila de prontas)
    if(disk_manager.status == SUSPEND)
    {
        //  acorda o gerente de disco e põe ele na fila de prontas)
        task_awake(&disk_manager, &sleeping_tasks_queue);
        #ifdef DEBUG
            printf("-> Acordou a tarefa gerenciadora de disco\n");
        #endif
    }
    
    // libera semáforo de acesso ao disco
    sem_up(&sem_disk);
 
    // suspende a tarefa corrente (retorna ao dispatcher)
    disk.signal = 0;

    #ifdef DEBUG
        printf("-> Liberou semaforo, resetou signal e suspendeu a tarefa colocando-a na fila do disco.\n");
    #endif
    task_suspend((task_t**)&disk_tasks_queue);

    #ifdef DEBUG
        printf("--DISK: Retornou sinal para tarefa com o TÉRMINO da OPERAÇÃO DE ESCRITA.\n");
    #endif
    
    return 0;
}