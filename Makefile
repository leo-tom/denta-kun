.SUFFIXES: .bo .o .c

INSTALL_PATH ?= /usr/local
DEBUG ?= 0
#DEBUGSTR != if [ $(DEBUG) -ge 1 ] ;  then  echo '-g -fsanitize=address -fno-omit-frame-pointer' ; else echo '-O3' ; fi;
DEBUGSTR != if [ $(DEBUG) -ge 1 ] ;  then  echo '-g -ggdb' ; else echo '-O3' ; fi;
OPTS1 =  $(DEBUGSTR) -I/usr/local/include -Wall -DDEBUG=$(DEBUG) -DBOOLEAN=0 -ldl
OPTS2 =  $(DEBUGSTR) -I/usr/local/include -Wall -DDEBUG=$(DEBUG) -DBOOLEAN=1 -ldl
SRC = main.c parser.c poly-funcs.c black-board.c builtin-funcs.c K.c bca.c pac.c fgen.c
OBJ1 = $(SRC:%.c=%.o)
OBJ2 = $(SRC:%.c=%.bo)
headers = denta-kun.h
CC = cc
RM = rm

all : dentakun bentakun

dentakun : $(OBJ1)
	$(CC) -o $@ $(OPTS1) -L/usr/local/lib -lgmp $(OBJ1)

bentakun : $(OBJ2)
	$(CC) -o $@ $(OPTS2) -L/usr/local/lib -lgmp $(OBJ2)

.c.bo : 
	$(CC) $(OPTS2) -c $< -o $@

.c.o :
	$(CC) $(OPTS1) -c $< -o $@

install : all
	install dentakun $(INSTALL_PATH)/bin/dentakun
	install bentakun $(INSTALL_PATH)/bin/bentakun
	install scripts/dentakun-tool.sh $(INSTALL_PATH)/bin/dentakun-tool
	install -d $(INSTALL_PATH)/share/dentakun
	cp -r scripts $(INSTALL_PATH)/share/dentakun/

uninstall : 
	$(RM) -f $(INSTALL_PATH)/bin/dentakun
	$(RM) -f $(INSTALL_PATH)/bin/bentakun
	$(RM) -f $(INSTALL_PATH)/bin/dentakun-tool
	$(RM) -f $(INSTALL_PATH)/share/dentakun

deinstall : uninstall

clean :
	$(RM) -f $(OBJ1) $(OBJ2)

