#
# Copyright (c) Tony Givargis, 2024-2025
#
# Makefile
#

AR     = ar
CC     = gcc
CFLAGS = -std=c99 -pedantic -Wall -Wextra -Werror -Wfatal-errors -O3
LDLIBS = -lm -lpthread
SRCS   = $(wildcard *.c)
OBJS   = $(SRCS:.c=.o)
DEST   = radium

MODULES = \
	utils/libutils.a \
	root/libroot.a

all: modules $(OBJS)
	$(CC) -O3 -o $(DEST) $(OBJS) $(MODULES) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<
	@$(CC) $(CFLAGS) -MM $< > $*.d

modules:
	@make -C utils
	@make -C root

clean:
	rm -f $(DEST) *.o *.d *~ *#
	@make -C utils clean > /dev/null
	@make -C root clean > /dev/null

-include $(OBJS:.o=.d)
