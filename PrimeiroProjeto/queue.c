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

    // By the statement of this exercise, wouldn't even exist a queue
    if(((queue->next = NULL) && (queue->prev == NULL)))
        return 0;
    
    // Check if the queue has only 1 element
    if( (queue->next == queue) && (queue->prev == queue))
        return  1;

    // Counts how many elements the queue has
    aux = queue;
    size = 0;
    
    while(aux->next != queue)
    {
        size++;
        aux = aux->next;
    }

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

    if(elem->next == NULL) || (elem->prev == NULL)
    {
        return -1; // Error: this element belongs to other queue and needs to be removed to be appended in this queue.
    }

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
    while(aux->next != *queue)
        aux = aux->next;
    // Now aux is the last node of the queue 

    // The new node receive the pointer "next" (the last node) of aux and the "prev" start to point to aux (the new penultimate of the queue)
    // Now, the aux is the penultimate, so it start to point to the element (the new last node)
    elem->prev = aux;    
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
    queue_t *aux;
    aux = *queue;

    if(*queue == NULL)
        return -1;

    // Removal of the first element of the queue
    if(aux->prev == aux->next)
    {
        aux->prev = NULL;
        aux->next = NULL;
        *queue == NULL;
        // To do: Verify the ptr_to_ptr   
    }

    // Removal of the last element of the queue


    aux = *queue;
    while(aux->next != elem)
        aux = aux->next;
    // Now, aux is the elem->prev

    // Removal in the middle 
    aux->next = elem->next;
    aux = aux->next;
    aux->prev = elem->prev;

    // Reset the pointers of elem
    elem->prev = NULL;
    elem->next = NULL;

    

    // O nó prev de "elem" passa a apontar para o nó next de "elem"
    // O nó next de "elem" passa a apontar para o next de "elem"


    return 1;
}

