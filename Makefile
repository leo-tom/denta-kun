DEBUG ?= 0
BOOLEAN ?= 0
DEBUGSTR != if [ $(DEBUG) -eq 1 ] ;  then  echo '-g -ggdb' ; else echo '-O2' ; fi;
options =  $(DEBUGSTR) -I/usr/local/include -Wall -DDEBUG=$(DEBUG) -DBOOLEAN=$(BOOLEAN)
objs = main.o parser.o poly-funcs.o black-board.o builtin-funcs.o K.o bca.o
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

bca.o : bca.c
	$(CC) $(options) -c $<

clean :
	$(RM) -f $(objs)
