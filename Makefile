# Makefile for Unix, using GNU C

#CC = gcc
#CFLAGS = -O2 -Wall -Wstrict-prototypes -ansi -pedantic 
#CFLAGS = -g
LFLAGS = -lm 

OBJS = rpn.o cmd.o

rpn: $(OBJS)
	$(CC) $(CFLAGS) -o rpn $(OBJS) $(LFLAGS)

clean:
	-rm -f rpn $(OBJS)

$(OBJS): rpn.h
