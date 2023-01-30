#include "util/hashmap.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "hashmap";

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

static uint64_t _hash(const char* key) {
    uint64_t hash = FNV_OFFSET;

    for (const char* p = key; *p; p++) {
        hash ^= (uint64_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }

    return hash;
}

void hashmap_init(hashmap_t** store, size_t bucket_count) {
    size_t sz = sizeof(hashmap_t) + (sizeof(hashmap_bucket_t*) * bucket_count);
    *store = (hashmap_t*)malloc(sz);

    if (*store != NULL) {
        (*store)->bucket_count = bucket_count;

        for (size_t i = 0; i < bucket_count; i++) {
            (*store)->buckets[i] = NULL;
        }
    }
}

void hashmap_insert(hashmap_t* store, const char* key, const char* value) {
    if (store == NULL)
        return;

    size_t index = _hash(key) % store->bucket_count;
    assert(index < store->bucket_count);

    hashmap_bucket_t* bucket = (hashmap_bucket_t*)malloc(sizeof(hashmap_bucket_t));
    if (bucket == NULL)
        return;

    bucket->key = (char*)malloc(strlen(key) + 1);
    bucket->value = (char*)malloc(strlen(value) + 1);
    bucket->next = NULL;

    if (bucket->value == NULL || bucket->key == NULL) {
        free(bucket);
        ESP_LOGE(TAG, "Coudln't alloc bucket values.");
        return;
    }

    strcpy(bucket->value, value);
    strcpy(bucket->key, key);

    if (store->buckets[index] == NULL) {
        store->buckets[index] = bucket;
    } else {
        hashmap_bucket_t* head = store->buckets[index];
        while (head->next != NULL) {
            head = head->next;
        }
        head->next = bucket;
    }
}

const char* hashmap_find(hashmap_t* store, const char* key) {
    if (store == NULL)
        return NULL;

    const char* value = NULL;
    size_t index = _hash(key) % store->bucket_count;
    assert(index < store->bucket_count);

    hashmap_bucket_t* head = store->buckets[index];

    while (head != NULL) {
        if (!strcmp(head->key, key)) {
            value = head->value;
            break;
        }
        head = head->next;
    }

    if (value == NULL) {
        ESP_LOGD(TAG, "Dict miss: %s", key);
    }

    return value;
}