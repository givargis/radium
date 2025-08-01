#
# Copyright (c) Tony Givargis, 2024-2025
# givargis@uci.edu
# Makefile
#

CC     = gcc
CFLAGS = -std=c99 -pedantic -Wall -Wextra -Werror -Wfatal-errors -O3
LDLIBS = -lm -lpthread
DEST   = radium

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

all: $(OBJS)
	$(CC) $(CFLAGS) -o $(DEST) $(OBJS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<
	@$(CC) $(CFLAGS) -MM $< > $*.d

clean:
	rm -f $(DEST) *.o *.d *~ *#

-include $(OBJS:.o=.d)
