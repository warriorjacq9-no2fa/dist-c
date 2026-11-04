CC = gcc
CFLAGS = -O0 -Wall -Werror -g

LD = $(CC)
LDFLAGS = -O0 -g

OBJS = \
ast.o \
parse.o \
main.o

.PHONY: clean

main: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -f $(OBJS) main