#ifndef __LIST_H__
#define __LIST_H__

struct node {
    char *file_name;
    char *channel_name;
    ssize_t file_size;
    struct node *next;
};
typedef struct node node_t;

void ll_print(node_t *head);
node_t *ll_find_tail(node_t *head);
void ll_free_all(node_t **head);
node_t *ll_add(node_t **head);

#endif
