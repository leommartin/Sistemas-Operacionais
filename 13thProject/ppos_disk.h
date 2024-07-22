// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// interface do gerente de disco rígido (block device driver)

#ifndef __DISK_MGR__
#define __DISK_MGR__

#define NO_OPERATION 0
#define WRITE_OPERATION 1
#define READ_OPERATION 2
#define OPERATION_DONE 1

// estruturas de dados e rotinas de inicializacao e acesso
// a um dispositivo de entrada/saida orientado a blocos,
// tipicamente um disco rigido.

// estrutura que representa um disco no sistema operacional
typedef struct
{
  int num_blocks;
  int block_size;
  int current_block;
  int signal;
  void* buffer; 

  // semaphore de acesso ao disco
  // status (livre, ocupado, leiutra, escrita)
  // sinal 
  // operaçao de leitura/escrita
  // buffer de leitura/escrita
  // fila de tarefas esperando acesso ao disco
  // tamanho de bloco em bytes
  // tamanho do disco em blocos
} disk_t ;

extern disk_t disk;

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize) ;

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer) ;

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer) ;

#endif