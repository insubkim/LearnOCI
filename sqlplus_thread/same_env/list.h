#ifndef LIST_H
#define LIST_H

#include <stdlib.h>
#include <string.h>

#define LIST_BUF_SIZE 1024

typedef struct s_list {
    char data[LIST_BUF_SIZE];
    int size;
    struct s_list *next;
} t_list;

// t_list* init_list(void);

t_list* push_back(t_list** head, char buf[1024]);

void    clear_list(t_list* head);

#endif