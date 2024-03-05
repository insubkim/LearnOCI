#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

typedef struct s_list {
    char data[1024];
    int size;
    struct s_list *next;
} t_list;

// t_list* init_list(void);

t_list* push_back(t_list** head, char buf[1024]);

void    clear_list(t_list* head);

#endif