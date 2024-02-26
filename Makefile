
CC = cc
CFLAGS = -ggdb -Wall 
INCLUDE = -I. -I$(ORACLE_HOME)/rdbms/public -I$(ORACLE_HOME)/network/public -I$(ORACLE_HOME)/plsql/public
DEFINES = -D_DEBUG

LIBS = -L$(ORACLE_HOME)/lib -lclntsh -lsqlplus 
LDFLAGS =

APPNAME = main
OBJECTS = main.o

all: $(APPNAME)

$(APPNAME): $(OBJECTS)
	$(CC) -o $@ $< $(LDFLAGS) $(LIBS)

test: all
	./$(APPNAME)

clean:
	-rm -f *.o

fclean: clean
	-rm $(APPNAME) *~

re : fclean
	$(all)

.PHONY: all test clean fclean re

.c.o:
	$(CC) -c $(CFLAGS) $(INCLUDE) $(DEFINES) -o $@ $<

.SUFFIXES: .c .o

