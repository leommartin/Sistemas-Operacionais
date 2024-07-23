// GRR20205675 Leonardo Marin Mendes Martin
// PingPongOS - PingPong Operating System
// Definição e operações em uma fila genérica.

#include "queue.h"
#include <stdio.h>
#include "ppos.h"
//------------------------------------------------------------------------------
// estrutura de uma fila genérica, sem conteúdo definido.
// Veja um exemplo de uso desta estrutura em testafila.c

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

void print_queue(queue_t *tasks_queue)
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

int queue_size (queue_t *queue) 
{
    queue_t *aux = queue;
    int size;

    // Check if the queue is empty
    if(queue == NULL)
        return 0;

    // By the statement of this exercise, wouldn't even exist a queue
    if(((queue->next == NULL) && (queue->prev == NULL)))
        return 0;
    
    // Check if the queue has only 1 element
    if((queue->next == queue) && (queue->prev == queue))
        return  1;

    // Counts how many elements the queue has
    size = 1;
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

void queue_print (char *name, queue_t *queue, void print_elem (void*)) 
{
    queue_t *aux = queue;

    // Print the string of testafila.c
    printf("%s: ", name);
    
    if(queue != NULL)
    {
        printf("[");
        print_elem((void*)aux);
    }
    else 
    {
        printf("[]\n");
        return ;
    }

    while(aux->next != queue)
    {
        aux = aux->next;
        printf(" ");
        print_elem((void*)aux);
    }

    printf("]\n");
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
    if((elem->prev != NULL) || (elem->next != NULL))
    {
        task_t *task = (task_t *)elem;
        printf("----ERROR: The element <%d> is invalid for queue_append(): the element already belongs to a queue.\n", task->id);
        printf("Fila que está tentando inserir o elemento: ");
        print_queue(*queue);
        printf("Fila de prontas: ");
        print_queue(ready_tasks_queue);
        printf("Fila de dormindo: ");
        print_queue((queue_t*)sleeping_tasks_queue);
        printf("Fila de tarefas do disco: ");
        print_queue((queue_t*)disk_tasks_queue);
        #ifdef DEBUG
            printf("----ERROR: The element is invalid for queue_append(): the element already belongs to a queue.\n");
        #endif
        return -1; // Error: this element belongs to other queue and needs to be removed to be appended in this queue.
    }
    
    // The queue is empty and we should to insert 1 element
    if(*queue == NULL)
    {
        (*queue) = elem;
        elem->prev = elem;
        elem->next = elem;
        return 0;
    } 

    // Find the last element in the queue
    queue_t *last = (*queue)->prev;

    // Adjust pointers
    last->next = elem;
    elem->prev = last;
    elem->next = *queue;
    (*queue)->prev = elem;
    
    return 0;
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
    int q_size;

    if(*queue == NULL || queue == NULL)
    {
        fprintf(stderr,"----ERROR: *queue points to NULL.\n");
        #ifdef DEBUG
            printf("----ERROR: *queue points to NULL.\n");
        #endif
        return -1;
    }

    if(elem == NULL)
    {
        fprintf(stderr,"----ERROR: The element is NULL.\n");
        return -1;
    }

    q_size = queue_size(*queue);
    if(q_size == 0)
    {
        fprintf(stderr,"----ERROR: The queue is empty.\n");
        return -1;
    }

    if((elem->prev == NULL) || (elem->next == NULL))
    {
        fprintf(stderr,"----ERROR: This element is invalid for queue_remove(): pointer to NULL.\n");
        return -1;
    }

    // Removal the first element of the queue
    if((elem->prev == (*queue)->prev) && (elem->next == (*queue)->next))
    {   
        // If there is just one element in the queue
        if(((*queue)->prev == (*queue)->next) && (q_size == 1))
        {
            elem->prev = NULL;
            elem->next = NULL;
            (*queue)->prev = NULL;
            (*queue)->next = NULL;
            (*queue) = NULL;
            queue = NULL;
            return 0;
        }

        // If there are many elements in the queue
        aux = (*queue)->prev;     
        (*queue)->prev->next = (*queue)->next;
        (*queue)->next->prev = aux;
        *queue = (*queue)->next;

        // Reset the element
        elem->prev = NULL;          
        elem->next = NULL;         

        return 0;
    }

    // Removal in the middle/end
    aux = *queue;
    int i = 1;
    while((aux->next != elem) && (i < q_size))
    {
        aux = aux->next;
        i++;
    }
    
    // Now, aux is on the left of the element that we want to remove
    if(aux->next == elem)
    {
        aux->next = elem->next;
        aux = aux->next;
        aux->prev = elem->prev;

        // Reset the pointers of elem
        elem->prev = NULL;
        elem->next = NULL;
        return 0;
    }
    
    fprintf(stderr,"----ERROR: The element don't belongs to this queue.\n");
    return -1;
}
