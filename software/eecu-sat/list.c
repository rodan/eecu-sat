
#include <stdlib.h>
#include <stdio.h>
#include "list.h"
#include "tlpi_hdr.h"


void ll_print(node_t *head)
{
    node_t *p = head;

    if (head == NULL) {
        printf("ll is empty\n");
        return;
    }

    while (NULL != p) {
        printf("n %p, [%s] sz %ld  next %p\n", (void *)p, p->file_name, p->file_size, (void *)p->next);
        if (p->next != NULL) {
            p = p->next;
        } else {
            return;
        }
    }
}

node_t *ll_find_tail(node_t *head)
{
    node_t *p = head;

    while (NULL != p) {
        if (p->next != NULL) {
            p = p->next;
        } else {
            return (p);
        }
    }

    return (p);
}

void ll_free_all(node_t **head)
{
    if (*head == NULL)
        return;
    node_t *p = *head;
    node_t *del;

    while (NULL != p) {
        //printf("remove node @%p\n", (void *)p);
        del = p;
        free(del->file_name);
        //del->thumb is freed in file_library()
        p = p->next;
        free(del);
    }

    *head = NULL;
}

node_t *ll_add(node_t **head)
{
    node_t *p = ll_find_tail(*head);
    node_t *new_node = NULL;

    new_node = (node_t *) calloc(1, sizeof(node_t));
    if (new_node == NULL) {
        errMsg("malloc error");
        return NULL;
    } else {
        //printf("new node @%p\n", (void *)new_node);
    }

    new_node->next = NULL;
    //new_node->value = val;

    if (p == NULL) {
        *head = new_node;
        //printf("new head @%p\n", (void *)new_node);
    } else {
        p->next = new_node;
        //printf("new node added to @%p\n", (void *)p);
    }

    return new_node;
}

