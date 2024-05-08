CC = gcc
CFLAGS = -Wall -g
# LDFLAGS = -pthread

# Lista de arquivos fonte
SRCS = ppos_core.c pingpong-wait.c queue.c

# Lista de arquivos objeto
OBJS = $(SRCS:.c=.o)

# Nome do executável de teste
EXEC = test_wait

# Regras de compilação
all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS += -DDEBUG
debug: all

zip:
	zip -r $(ZIPFILE) *

.PHONY: all debug zip

clean:
	rm -f $(OBJS) $(EXEC) $(ZIPFILE)
