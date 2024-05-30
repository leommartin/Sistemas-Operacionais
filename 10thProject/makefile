CC = gcc
CFLAGS = -Wall -g

# Lista de arquivos fonte para o tetse pingpong-semaphore.c
SRCS = ppos_core.c pingpong-semaphore.c queue.c ppos_ipc.c

# Lista de arquivos fonte para o teste pingpong-racecond.c
SRCS_RACE = ppos_core.c pingpong-racecond.c queue.c ppos_ipc.c

# Lista de arquivos objeto
OBJS = $(SRCS:.c=.o)

# Lista de arquivos objeto
OBJS_RACE = $(SRCS_RACE:.c=.o)

# Nome do executável de teste pingpong-semaphore.c
EXEC = test_semaphore

# Nome do executável de teste pingpong-racond.c
EXEC_RACE = test_racecond

# Nome do arquivo compactado
ZIPFILE = P10.zip

# Regras de compilação
all: $(EXEC) $(EXEC_RACE)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

$(EXEC_RACE): $(OBJS_RACE)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS += -DDEBUG
debug: $(EXEC) $(EXEC_RACE)
# debug: all

zip:
	zip -r $(ZIPFILE) *

.PHONY: all debug zip

clean:
	rm -f $(OBJS) $(OBJS_RACE) $(EXEC) $(EXEC_RACE) $(ZIPFILE)
