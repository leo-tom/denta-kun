#options = -02 -Wall
options = -g -ggdb -Wall -DDEBUG=0
objs = main.o parser.o poly-funcs.o black-board.o
headers = denta-kun.h
CC = gcc
RM = rm

all : denta-kun

denta-kun : $(objs)
	$(CC) -o $@ $(options) $(objs)

main.o : main.c
	$(CC) $(options) -c $<

parser.o : parser.c
	$(CC) $(options) -c $<
	
poly-funcs.o : poly-funcs.c
	$(CC) $(options) -c $<

black-board.o : black-board.c
	$(CC) $(options) -c $<

clean :
	$(RM) $(objs)
