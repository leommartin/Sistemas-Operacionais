#include "ppos.h"
#include "queue.h"
#include "disk.h"
#include "ppos_disk.h"
#include <stdio.h>

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
    // if(block < 0 || block >= disk.num_blocks)
    // {
    //     fprintf(stderr, "Error: block out of range\n");
    //     return -1;
    // }
    
        
    // // obtém o semáforo de acesso ao disco
    // sem_down(&sem_disk);
 
    // // inclui o pedido na fila do disco
    // current_task->rw_operation = READ_OPERATION;

    // // -> Se o disco não estiver livre, suspende a tarefa corrente e coloca na fila do disco
    // if(disk_cmd (DISK_CMD_STATUS, 0, 0) != DISK_STATUS_IDLE)
    // {
    //     queue_append((queue_t**)disk_tasks_queue, (queue_t*) current_task);
    // }
    
    // // To do: verificar se o pedido de leitura realmente é feito aqui
    // if(disk_cmd(DISK_CMD_READ, block, buffer) < 0)
    // {
    //     return -1;
    // }
    
    // // se o gerente de disco está dormindo
    // //   -> acorda o gerente de disco (põe ele na fila de prontas)
    // if(disk_manager.status == SUSPEND)
    // {
    //     //  acorda o gerente de disco e põe ele na fila de prontas)
    //     task_awake(&disk_manager, &sleeping_tasks_queue);
    // }
 
    // // libera semáforo de acesso ao disco
    // sem_up(&sem_disk);

    // // suspende a tarefa corrente (retorna ao dispatcher)
    // current_task->rw_operation = NO_OPERATION;
    // task_suspend((task_t**) ready_tasks_queue);

    return 0;

}

int disk_block_write (int block, void* buffer) 
{
    // // obtém o semáforo de acesso ao disco
    // sem_down(&sem_disk);

    // // inclui o pedido na fila_disco
    // if(disk_cmd (DISK_CMD_STATUS, 0, 0) != DISK_STATUS_IDLE)
    // {
    //     queue_append((queue_t**)disk_tasks_queue, (queue_t*) current_task);
    // }
    // else
    // {
    //     if(disk_cmd(DISK_CMD_WRITE, block, buffer) < 0)
    //     {
    //         return -1;
    //     }
    // }
 
    // // se o gerente de disco está dormindo
    // //   -> acorda o gerente de disco (põe ele na fila de prontas)
    // if(disk_manager.status == SUSPEND)
    // {
    //     //  acorda o gerente de disco e põe ele na fila de prontas)
    //     task_awake(&disk_manager, &sleeping_tasks_queue);
    // }
    
    // // libera semáforo de acesso ao disco
    // sem_up(&sem_disk);
 
    // // suspende a tarefa corrente (retorna ao dispatcher)
    // current_task->rw_operation = NO_OPERATION;
    // task_suspend((task_t**) ready_tasks_queue);
    
    return 0;
}