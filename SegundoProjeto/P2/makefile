# Nome do executável
EXECUTABLE = test_contexts

# Compilador
CC = gcc

# Flags de compilação
CFLAGS = -Wall -Wextra -std=c99

# Flag de debug
DEBUG_FLAGS = -g

# Arquivos fonte
SOURCES = ppos_core.c

# Arquivos header
HEADERS = ppos.h ppos_data.h

# Objetos
OBJECTS = $(SOURCES:.c=.o)

# Regras de compilação
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $@ $^

# Regra de compilação para objetos
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c -o $@ $<

# Limpeza
clean:
	rm -f $(EXECUTABLE) $(OBJECTS)
