#
# Copyright (c) Tony Givargis, 2020-2025
#
# Makefile
#

AR      = ar
ARFLAGS = rcs
CC      = gcc
CFLAGS  = -std=c99 -pedantic -Wall -Wextra -Werror -Wfatal-errors -fPIC -O3
SRCS    = $(wildcard *.c)
OBJS    = $(SRCS:.c=.o)
DEST    = libkernel.a

all: $(OBJS)
	@echo "[AR]" $(DEST)
	@$(AR) $(ARFLAGS) $(DEST) $(OBJS)

%.o: %.c
	@echo "[CC]" $<
	@$(CC) $(CFLAGS) -c $<
	@$(CC) $(CFLAGS) -MM $< > $*.d

clean:
	@rm -f $(DEST) *.so *.a *.o *.d *~ *#

-include $(OBJS:.o=.d)
