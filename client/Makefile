CC := gcc
CFLAGS = -Wall -g -Os

SHDIR = ../common

OBJS = client.o $(SHDIR)/common.o

all: client

client: $(OBJS)
	@$(CC) -o client $(CFLAGS) $(OBJS)

$(OBJS) : %.o: %.c
	@$(CC) -c $(CFLAGS) $< -o $@

.PHONY:
clean:
	@rm -f *.o client
	@rm -f ../common/*.o
	@echo Done cleaning
