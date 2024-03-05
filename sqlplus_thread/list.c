#include "list.h"

// //init
// t_list* init_list(void) {
//     t_list* list;

//     list = malloc(sizeof list);
//     if (list == NULL) return NULL;

//     list->data[0] = 0;
//     list->size = 0;
//     list->next = 0;
//     return list;
// }

//add list
t_list* push_back(t_list** head, char buf[1024]) {
    t_list* list;

    list = malloc(sizeof *list);
    if (list == NULL) return NULL;
    
    int size = strlen(buf);
    list->size = size;
    list->next = NULL;
    memcpy(list->data, buf, size);
    list->data[size] = 0;
    
    if (*head) {
        t_list* node = *head;
        while (node->next) node = node->next;
        node->next = list; 
    } else {
        *head = list;
    }
    return list;
}

//clear
void    clear_list(t_list* head) {
    while (head) {
        t_list* tmp;
        tmp = head;
        head  = head->next;
        free(tmp);
    }
}