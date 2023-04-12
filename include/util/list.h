#ifndef __list_h
#define __list_h

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LIST_FOREACH(root, ptr) for (ptr = root; ptr != NULL; ptr = ptr->next)
#define LIST_ADD(root, item)                                                                       \
    {                                                                                              \
        item->next = root;                                                                         \
        root = item;                                                                               \
    }

struct list_node {
    void* data;
    struct list_node* next;
};

typedef struct list_node list_node_t;

struct list {
    struct list_node* _first;
    struct list_node* _last;
};

typedef struct list list_t;

#define LIST_DEFAULT()                                                                             \
    { NULL, NULL }

// #define list_foreach(list, type, item)
//     for (type* item = NULL, *_ = NULL; _ == NULL; _ = 1)
//         for (list_node_t* ptr = list._first; ptr != NULL; ptr = ptr->next)
//             if ((item = (type*)ptr->data) != NULL)

#define list_foreach(list, item)                                                                   \
    for (list_node_t* ptr = list._first; ptr != NULL; ptr = ptr->next)                             \
        if ((item = ptr->data) != NULL)

list_node_t* list_add(list_t* list, void* data);
void list_remove(list_t* list, void* data);

#ifdef __cplusplus
}
#endif

#endif
