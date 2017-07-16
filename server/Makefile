CC := gcc
CFLAGS = -Wall -g -Os

SHDIR = ../common

OBJS = server.o $(SHDIR)/common.o

all: server

server: $(OBJS)
	@$(CC) -o server $(CFLAGS) $(OBJS)

$(OBJS) : %.o: %.c
	@$(CC) -c $(CFLAGS) $< -o $@

.PHONY:
clean:
	@rm -f *.o server
	@rm -f ../common/*.o
	@echo Done cleaning
