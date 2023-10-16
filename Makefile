SRCS := servermain.cpp client.cpp
OBJS := $(SRCS:.cpp=.o)

# Compiler and compilation flags
CC := g++
CFLAGS := -g -Wall

all: servermain client

servermain: servermain.o
	$(CC) $(CFLAGS) -o $@ $^

client: client.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) servermain client $(OBJS)
