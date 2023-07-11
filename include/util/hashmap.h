#ifndef __util__hashmap_h
#define __util__hashmap_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct hashmap_bucket {
    struct hashmap_bucket* next;
    char* key;
    char* value;
} hashmap_bucket_t;

typedef struct hashmap {
    size_t bucket_count;
    hashmap_bucket_t* buckets[];
} hashmap_t;

void hashmap_init(hashmap_t** store, size_t bucket_count);
void hashmap_insert(hashmap_t* store, const char* key, const char* value);
const char* hashmap_find(hashmap_t* store, const char* key);

#ifdef __cplusplus
}
#endif

#endif
