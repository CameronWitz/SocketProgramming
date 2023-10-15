SRCS := servermain.c client.c
OBJS := $(SRCS:.c=.o)

# Compiler and compilation flags
CC := gcc
CFLAGS := -g -Wall

all: servermain client

servermain: servermain.o
	$(CC) $(CFLAGS) -o $@ $^

client: client.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) servermain client $(OBJS)
