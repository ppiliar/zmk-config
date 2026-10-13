// Simulator stub for <zephyr/sys/slist.h> — plain double pointer nodes.
#pragma once

#include <stddef.h>

struct _snode {
    struct _snode *next;
};
typedef struct _snode sys_snode_t;

struct _slist {
    sys_snode_t *head;
    sys_snode_t *tail;
};
typedef struct _slist sys_slist_t;

#define SYS_SLIST_STATIC_INIT(ptr_to_list) {NULL, NULL}

static inline void sys_slist_append(sys_slist_t *list, sys_snode_t *node) {
    node->next = NULL;
    if (list->tail == NULL) {
        list->head = node;
    } else {
        list->tail->next = node;
    }
    list->tail = node;
}

// Matches zephyr list_gen.h semantics: user code supplies the enclosing braces.
#define SIM_SLIST_CONTAINER(node, cn, n) CONTAINER_OF(node, __typeof__(*cn), n)

#define SYS_SLIST_FOR_EACH_CONTAINER(sl, cn, n)                                                    \
    for ((cn) = ((sl)->head ? SIM_SLIST_CONTAINER((sl)->head, cn, n) : NULL); (cn) != NULL;        \
         (cn) = ((cn)->n.next ? SIM_SLIST_CONTAINER((cn)->n.next, cn, n) : NULL))

// Zephyr's CONTAINER_OF
#define CONTAINER_OF(ptr, type, field)                                                             \
    ((type *)(((char *)(ptr)) - offsetof(type, field)))
