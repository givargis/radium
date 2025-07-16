#
# Copyright (c) Tony Givargis, 2020-2025
#
# Makefile
#

CC     = gcc
CFLAGS = -std=c99 -pedantic -Wall -Wextra -Werror -Wfatal-errors -fPIC -O3
LDLIBS = -lm -ldl -lpthread
SRCS   = $(wildcard *.c)
OBJS   = $(SRCS:.c=.o)
DEST   = radium

MODULES = \
	lang/liblang.a \
	index/libindex.a \
	utils/libutils.a \
	kernel/libkernel.a

all: modules $(OBJS)
	@echo "[LN]" $(DEST)
	@$(CC) -O3 -o $(DEST) $(OBJS) $(MODULES) $(LDLIBS)

%.o: %.c
	@echo "[CC]" $<
	@$(CC) $(CFLAGS) -c $<
	@$(CC) $(CFLAGS) -MM $< > $*.d

modules:
	@make -C lang
	@make -C index
	@make -C utils
	@make -C kernel

clean:
	@rm -f $(DEST) *.so *.a *.o *.d *~ *#
	@make -C lang clean > /dev/null
	@make -C index clean > /dev/null
	@make -C kernel clean > /dev/null
	@make -C utils clean > /dev/null

-include $(OBJS:.o=.d)
