# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g

# Source files
SRCS = ppos_core.c pingpong-tasks1.c

# Object files
OBJS = $(SRCS:.c=.o)

# Dependency files
DEPS = ppos.h ppos_data.h

# Executable
EXEC = test_contexts

# Default target
all: $(EXEC)

# Rule to generate executable
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Rule to compile C source files
%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

# Phony target to clean up object files and executable
clean:
	rm -f $(OBJS) $(EXEC)

# Debug target
debug: CFLAGS += -DDEBUG
debug: all
