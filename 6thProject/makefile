CC = gcc
CFLAGS = -Wall -g
# LDFLAGS = -pthread

# Lista de arquivos fonte para o teste pingpong-contab.c
SRCS = ppos_core.c pingpong-contab.c queue.c

# Lista de arquivos fonte para o teste pingpong-contab-prio.c
SRCS_PRIO = ppos_core.c pingpong-contab-prio.c queue.c

# Lista de arquivos objeto para o teste pingpong-contab.c
OBJS = $(SRCS:.c=.o)

# Lista de arquivos objeto para o teste pingpong-contab-prio.c
OBJS_PRIO = $(SRCS_PRIO:.c=.o)

# Nome do executável de teste pingpong-contab.c
EXEC = test_contab

# Nome do executável de teste pingpong-contab-prio.c
EXEC_PRIO = test_contab_prio

# Nome do arquivo zip
ZIPFILE = P6.zip

# Regras de compilação
all: $(EXEC) $(EXEC_PRIO)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

$(EXEC_PRIO): $(OBJS_PRIO)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS += -DDEBUG
debug: $(EXEC) $(EXEC_PRIO)

zip:
	zip -r $(ZIPFILE) *

.PHONY: all debug zip

clean:
	rm -f $(OBJS) $(OBJS_PRIO) $(EXEC) $(EXEC_PRIO) $(ZIPFILE)
