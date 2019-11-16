
options = -g -ggdb -Wall
objs = main.o poly-funcs.o
CC = gcc
RM = rm

all : denta-kun

denta-kun : $(objs)
	$(CC) -o $@ $(options) $(objs)
	
main.o : main.c
	$(CC) $(options) -c $<
	
poly-funcs.o : poly-funcs.c
	$(CC) $(options) -c $<

clean :
	$(RM) $(objs)
