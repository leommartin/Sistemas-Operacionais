void handler(int signum)
{
    if (signum == SIGALRM) 
    {
        // Lógica existente para SIGALRM
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
    } 
    
    if (signum == SIGUSR1) 
    {
        // Acorda a tarefa gerenciadora de disco
        if (disk_manager.status == SUSPEND) {
            task_awake(&disk_manager, &sleeping_tasks_queue);
        }
    }

    program_timer++;

}

void set_signal_handlers() {
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGALRM, &action, NULL) < 0) {
        perror("Erro em sigaction SIGALRM: ");
        exit(1);
    }

    if (sigaction(SIGUSR1, &action, NULL) < 0) {
        perror("Erro em sigaction SIGUSR1: ");
        exit(1);
    }
}