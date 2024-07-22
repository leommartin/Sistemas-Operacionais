// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.5 -- Março de 2023

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include <signal.h>     // biblioteca para tratar sinais
#include <sys/time.h>   // biblioteca para gerar os clock ticks
#include "queue.h"

#define STACKSIZE 64 * 1024 /* tamanho de pilha das threads */
#define ID_MAIN 0
#define SYSTEM_TASK 0
#define QUEUE_STATUS_DEAD 0
#define QUEUE_STATUS_ALIVE 1
#define USER_TASK 1
#define READY 2
#define RUNNING 3
#define SUSPEND 4
#define TERMINATED 5
#define QUANTUM 10

typedef struct cpu_timer
{
  unsigned int start_time;
  unsigned int end_time;
} cpu_timer;

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
  struct task_t *prev, *next ;		// ponteiros para usar em filas
  int id ;				// identificador da tarefa
  ucontext_t context ;			// contexto armazenado da tarefa
  short status ;			// pronta, rodando, suspensa, ...
  int static_prio;
  int dynamic_prio;
  int type;
  int quantum_timer;
  int execution_time;
  int processing_time;
  int num_activations;
  struct task_t *wait_queue;
  int wake_up_time;
  int rw_operation;
  // ... (outros campos serão adicionados mais tarde)
} task_t ;

extern queue_t *ready_tasks_queue;
extern task_t *current_task;
extern task_t *sleeping_tasks_queue;
extern task_t disk_manager;
extern task_t *disk_tasks_queue;

// estrutura que define um semáforo
typedef struct
{
  int counter;
  task_t *queue;
  // preencher quando necessário
} semaphore_t ;

extern semaphore_t sem_disk;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

typedef struct mqueue_msg_t
{
  struct mqueue_msg_t *prev, *next ;	// ponteiros para usar em filas
  void *content; // conteúdo da mensagem
} mqueue_msg_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  mqueue_msg_t *buffer; // buffer da fila
  semaphore_t s_buffer, s_item, s_vaga; // semáforos para controle de acesso à fila
  int max_msgs; // capacidade da fila (número de floats/ints/chars)
  int msg_size; // tamanho de cada mensagem(float, int, char, etc)
  int size; // número de mensagens na fila
  int status; // status da fila
} mqueue_t ;

#endif

