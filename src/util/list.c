#include "util/list.h"
#include "esp_log.h"
#include <stdlib.h>

static const char* TAG = "list";

list_node_t* list_add(list_t* list, void* data) {
    list_node_t* node = (list_node_t*)malloc(sizeof(list_node_t));

    if (node == NULL) {
        return node;
    }

    node->data = data;
    node->next = NULL;

    if (list->_first == NULL) {
        list->_first = node;
    }

    if (list->_last != NULL) {
        list->_last->next = node;
    }

    list->_last = node;
    return node;
}

void list_remove(list_t* list, void* data) {
    ESP_LOGI(TAG, "Remove from list: %p", data);
    
    if (list == NULL || data == NULL) {
        return;
    }
    
    list_node_t* ptr = list->_first;
    list_node_t* prev = NULL;

    while (ptr != NULL) {
        if (ptr->data == data) {
            // Update previous node's next pointer
            if (prev != NULL) {
                prev->next = ptr->next;
            } else {
                // Removing first node
                list->_first = ptr->next;
            }
            
            // Update last pointer if we're removing the last node
            if (ptr == list->_last) {
                list->_last = prev;
            }
            
            free(ptr);
            return;
        }
        
        prev = ptr;
        ptr = ptr->next;
    }
}