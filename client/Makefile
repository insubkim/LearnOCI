NAME=client

CC=gcc

CFLAGS=-g #-Wall -Wextra -Werror
LIB=-lpthread

SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)

%.o : %.c 
	$(CC) $(CFLAGS) $(LIB) $< -c

all : $(NAME)

$(NAME) : $(OBJ)
	$(CC) -o $(NAME)  $(OBJ)  $(CFLAGS) $(LIB)

re : fclean
	$(NAME) 

clean : 
	rm -rf $(OBJ) 

fclean : clean
	rm -rf $(NAME) 

.PHONY: re clean fclean