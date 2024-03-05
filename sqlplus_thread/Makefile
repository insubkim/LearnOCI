NAME=server

CC=gcc

CFLAGS=-g #-Wall -Wextra -Werror
LIB=-lpthread -L$(ORACLE_HOME)/lib -lclntsh -lsqlplus 
INCLUDE = -I. -I$(ORACLE_HOME)/rdbms/public -I$(ORACLE_HOME)/network/public -I$(ORACLE_HOME)/plsql/public

SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)

%.o : %.c 
	$(CC) $(CFLAGS) $(LIB) $(INCLUDE) $< -c

all : $(NAME)

$(NAME) : $(OBJ)
	$(CC) -o $(NAME)  $(OBJ)  $(CFLAGS) $(LIB)

re : fclean
	make all

clean : 
	rm -rf $(OBJ) 

fclean : clean
	rm -rf $(NAME) 

.PHONY: re clean fclean