CC = gcc
CFLAGS = -O2 -Wall -Werror -g

LD = $(CC)
LDFLAGS = -O2 -g

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