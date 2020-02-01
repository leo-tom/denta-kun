DEBUG ?= 0
#options = -g -ggdb -I/usr/local/include -Wall -DDEBUG=$(DEBUG)
options = -O2 -I/usr/local/include -Wall -DDEBUG=$(DEBUG)
objs = main.o parser.o poly-funcs.o black-board.o builtin-funcs.o K.o
headers = denta-kun.h
CC = cc
RM = rm

all : dentakun

dentakun : $(objs)
	$(CC) -o $@ $(options) -L/usr/local/lib -lgmp $(objs)

main.o : main.c
	$(CC) $(options) -c $<

parser.o : parser.c
	$(CC) $(options) -c $<
	
poly-funcs.o : poly-funcs.c
	$(CC) $(options) -c $<

black-board.o : black-board.c
	$(CC) $(options) -c $<

builtin-funcs.o : builtin-funcs.c
	$(CC) $(options) -c $<

K.o : K.c
	$(CC) $(options) -c $<

clean :
	$(RM) $(objs)
