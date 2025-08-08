#
# Copyright (c) Tony Givargis, 2024-2025
# givargis@uci.edu
# Makefile
#

CC     = gcc
CFLAGS = -std=c99 -pedantic -Wall -Wextra -Werror -Wfatal-errors -O3
LDLIBS = -lm -lpthread
LIBS   = libra/libra.a
DEST   = radium

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

all: libs $(OBJS)
	$(CC) $(CFLAGS) -o $(DEST) $(OBJS) $(LIBS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<
	@$(CC) $(CFLAGS) -MM $< > $*.d

libs:
	@make -C libra

clean:
	rm -f $(DEST) *.o *.d *~ *#
	@make -C libra clean

-include $(OBJS:.o=.d)
