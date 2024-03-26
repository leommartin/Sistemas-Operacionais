CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -pthread

# Lista de arquivos fonte
SRCS = ppos_core.c pingpong-dispatcher.c

# Lista de arquivos objeto
OBJS = $(SRCS:.c=.o)

# Nome do executável de teste
EXEC1 = task_queue

# Regras de compilação
all: $(EXEC1)

$(EXEC1): ppos_core.o pingpong-dispatcher.o
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS += -DDEBUG
debug: all

clean:
	rm -f $(OBJS) $(EXEC1)

.PHONY: all debug clean
