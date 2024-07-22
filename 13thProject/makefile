# Compilador e flags
CC = gcc
CFLAGS = -Wall -g

# Arquivos fontes e objetos
SRC = ppos_core.c ppos_disk.c queue.c ppos_ipc.c disk.c
OBJ = $(SRC:.c=.o)

# Executáveis
EXEC1 = test1_disk
EXEC2 = test2_disk

# Regras de compilação
all: $(EXEC1) $(EXEC2)

# Compila o executável test1_disk
$(EXEC1): pingpong-disco1.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ pingpong-disco1.o $(OBJ) -lrt -lm

# Compila o executável test2_disk
$(EXEC2): pingpong-disco2.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ pingpong-disco2.o $(OBJ) -lrt -lm

# Regras de criação dos arquivos objetos
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpar arquivos objetos e executáveis
clean:
	rm -f *.o $(EXEC1) $(EXEC2)

.PHONY: all clean
