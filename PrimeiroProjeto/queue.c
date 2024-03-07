// PingPongOS - PingPong Operating System
// Leonardo Marin Mendes Martin, DINF UFPR
// Definição e operações em uma fila genérica.

#include "queue.h"
#include <stdio.h>
//------------------------------------------------------------------------------
// estrutura de uma fila genérica, sem conteúdo definido.
// Veja um exemplo de uso desta estrutura em testafila.c

// typedef struct queue_t
// {
//    struct queue_t *prev ;  // aponta para o elemento anterior na fila
//    struct queue_t *next ;  // aponta para o elemento seguinte na fila
// } queue_t ;

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size (queue_t *queue) 
{
    queue_t *aux;
    int size;

    // Check if the queue is empty
    if(queue == NULL)
        return 0;

    if(((queue->next = NULL) && (queue->prev == NULL)))
        return 0;
    
    // Check if the queue has only 1 element
    if( (queue->next == queue) && (queue->prev == queue))
        return  1;

    // Counts how many elements the queue has
    aux = queue;
    size = 0;
    do 
    {
        size++;
        aux = aux->next;
    }while(aux->next != queue);

    return size;
}

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca. Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir

void queue_print (char *name, queue_t *queue, void print_elem (void*) ) 
{
    printf("Função a ser feita");
}

//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila
// Retorno: 0 se sucesso, <0 se ocorreu algum erro

int queue_append (queue_t **queue, queue_t *elem) 
{

    queue_t *aux;

    // The queue is empty and we should to insert 1 element
    if(*queue == NULL)
    {
        *queue = elem;
        elem->prev = elem;
        elem->next = elem;
        return 1;
    }   

    // The queue has only 1 element
    if((*queue->next ==  *queue) (*queue->prev == *queue))
    {
        elem->prev = aux;
        elem->next = aux;
        aux->next = elem;
        aux->prev = elem;
        return 1;
    }

    // The queue has 2+ elements
    aux = *queue;
    while(aux->next != aux->prev)
        aux = aux->next;
    // Now aux is the last node of the queue 
    elem->prev = aux->prev;    
    elem->next = aux->next;
    aux->next = elem;
    
    return 1;
}

//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: 0 se sucesso, <0 se ocorreu algum erro

int queue_remove (queue_t **queue, queue_t *elem) 
{
    printf("Função a ser feita");
    return 1;
}

