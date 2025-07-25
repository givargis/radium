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
DEST   = libutils.a

all: $(OBJS)
	$(AR) rcs $(DEST) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<
	@$(CC) $(CFLAGS) -MM $< > $*.d

clean:
	rm -f $(DEST) *.o *.d *~ *#

-include $(OBJS:.o=.d)
