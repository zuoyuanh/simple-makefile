CC = gcc
CFLAGS = -Wall -ansi -pedantic
MAIN = simple_make
OBJS = simple_make.o
all : $(MAIN)

$(MAIN) : $(OBJS)
	$(CC) $(CFLAGS) -o $(MAIN) $(OBJS)

smake.o : smake.c
	$(CC) $(CFLAGS) -c simple_make.c

clean :
	rm *.o $(MAIN)
