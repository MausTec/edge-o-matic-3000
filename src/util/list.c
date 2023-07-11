#include "util/list.h"
#include "esp_log.h"

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
    
    list_node_t* ptr = list->_first;
    list_node_t* prev = NULL;

    while (ptr != NULL) {
        if (ptr->data == data) {
            if (prev != NULL) {
                prev->next = ptr->next;
            }

            if (ptr == list->_first) {
                list->_first = ptr->next;
            }

            if (ptr == list->_last) {
                list->_last = prev;
            }
        } else {
            prev = ptr;
        }

        ptr = ptr->next;
    }
}