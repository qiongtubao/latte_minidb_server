

#ifndef __LATTE_UTILS_LRU_CACHE_H
#define __LATTE_UTILS_LRU_CACHE_H

#include "dict/dict.h"
#include "list/list.h"
typedef struct keyValue {
    void* key;
    void* value;
} keyValue;
keyValue* keyValueCreate(void* key, void* value);
void keyValueRelease(keyValue* kv);


typedef struct lruCache
{
    list list;
    dict* searcher;
} lruCache;
lruCache* lruCacheCreate(int size);
/**
 *  put 1
 *  update 0
 */
int lruCache_put(lruCache* cache, void* key, void* value);
void* lruCache_get(lruCache* cache, void* key);
void lruCache_remove(lruCache* cache, void* key);

#endif