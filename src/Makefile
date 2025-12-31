# Copyright (c) Tony Givargis, 2024-2026

CC     = gcc
CFLAGS = -ansi -pedantic -Wall -Wextra -Werror -Wfatal-errors -O3
LDLIBS = -lm -lpthread
LIBS   = core/libcore.a
DEST   = radium

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

all: libs $(OBJS)
	@echo "[LN] $(DEST)"
	@$(CC) $(CFLAGS) -o $(DEST) $(OBJS) $(LIBS) $(LDLIBS)

%.o: %.c
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -c $<
	@$(CC) $(CFLAGS) -MM $< > $*.d

libs:
	@make -C core

clean:
	@rm -f $(DEST) *.o *.d *~ *#
	@make -C core clean

-include $(OBJS:.o=.d)
