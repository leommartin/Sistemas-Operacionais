CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -pthread

# Lista de arquivos fonte
SRCS = ppos_core.c pingpong-tasks1.c pingpong-tasks2.c pingpong-tasks3.c

# Lista de arquivos objeto
OBJS = $(SRCS:.c=.o)

# Nome do executável de teste
EXEC1 = test_context1
EXEC2 = test_context2
EXEC3 = test_context3

# Regras de compilação
all: $(EXEC1) $(EXEC2) $(EXEC3)

$(EXEC1): ppos_core.o pingpong-tasks1.o
	$(CC) $(LDFLAGS) $^ -o $@

$(EXEC2): ppos_core.o pingpong-tasks2.o
	$(CC) $(LDFLAGS) $^ -o $@

$(EXEC3): ppos_core.o pingpong-tasks3.o
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS += -DDEBUG
debug: all

clean:
	rm -f $(OBJS) $(EXEC1) $(EXEC2) $(EXEC3)

.PHONY: all debug clean
