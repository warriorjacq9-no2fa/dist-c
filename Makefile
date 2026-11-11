CC = gcc
CFLAGS = -O2 -Wall -Werror -g

LD = $(CC)
LDFLAGS = -O2
LIBS = 

OBJS = \
parser.o \
lexer.o \
ast.o \
main.o

.PHONY: all clean

all: main

main: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -rf $(OBJS) main