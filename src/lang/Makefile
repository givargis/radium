# Copyright (c) Tony Givargis, 2024-2026

AR     = ar
CC     = gcc
CFLAGS = -ansi -pedantic -Wall -Wextra -Werror -Wfatal-errors -O3
DEST   = liblang.a

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

all: $(OBJS)
	@echo "[AR] $(DEST)"
	@$(AR) rcs $(DEST) $(OBJS)

%.o: %.c
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -c $<
	@$(CC) $(CFLAGS) -MM $< > $*.d

clean:
	@rm -f $(DEST) *.o *.d *~ *#

-include $(OBJS:.o=.d)
